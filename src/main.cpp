#include <memory>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "sensesp_app_builder.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/signalk/signalk_ws_client.h"
#include "sensesp/ui/config_item.h"
#include "sensesp/ui/ui_controls.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_value_listener.h"
#include "sensesp/transforms/lambda_transform.h"
#include "sensesp/system/valueconsumer.h"
#include "sensesp/system/hash.h"

#include "pin_config.h"
#include "winch_controller.h"
#include "home_sensor.h"
#include "automatic_mode_controller.h"
#include "remote_control.h"

#include "secrets.h"

#ifndef AP_PASSWORD
#error "AP_PASSWORD not defined. Define it in src/secrets.h."
#endif

#ifndef OTA_PASSWORD
#error "OTA_PASSWORD not defined. Define it in src/secrets.h."
#endif

using namespace sensesp;

namespace {
    bool findConfigFile(const String& config_path, String& filename) {
        const String hash_path = String("/") + Base64Sha1(config_path);
        const String paths_to_check[] = {hash_path, hash_path + "\n"};

        for (const auto& path : paths_to_check) {
            if (SPIFFS.exists(path)) {
                filename = path;
                return true;
            }
        }

        if (config_path.length() < 32 && SPIFFS.exists(config_path)) {
            filename = config_path;
            return true;
        }

        return false;
    }

    void updateApPasswordIfDefault(const char* ap_password) {
        const String config_path = "/System/WiFi Settings";
        String filename;
        if (!findConfigFile(config_path, filename)) {
            return;
        }

        File file = SPIFFS.open(filename, "r");
        if (!file) {
            return;
        }

        DynamicJsonDocument doc(1024);
        auto error = deserializeJson(doc, file);
        file.close();
        if (error) {
            return;
        }

        bool updated = false;
        const String default_password = "thisisfine";

        if (doc["apSettings"].is<JsonVariant>()) {
            JsonObject ap_settings = doc["apSettings"].as<JsonObject>();
            String current = ap_settings["password"] | "";
            if (current.length() == 0 || current == default_password) {
                ap_settings["password"] = ap_password;
                updated = true;
            }
        } else if (doc["ap_mode"].is<String>()) {
            String ap_mode = doc["ap_mode"].as<String>();
            String current = doc["password"] | "";
            if ((ap_mode == "Access Point" || ap_mode == "Hotspot") &&
                (current.length() == 0 || current == default_password)) {
                doc["password"] = ap_password;
                updated = true;
            }
        }

        if (!updated) {
            return;
        }

        File out = SPIFFS.open(filename, "w");
        if (!out) {
            return;
        }
        serializeJson(doc, out);
        out.close();
    }
}

// Global system components
WinchController winch_controller;
HomeSensor home_sensor;
AutomaticModeController* auto_mode_controller = nullptr;
RemoteControl* remote_control = nullptr;

// Global Variables
volatile long pulse_count = 0;              // Bidirectional pulse counter (ISR access)
float config_meters_per_pulse = 0.01;       // Configurable conversion factor
SKOutputFloat* auto_mode_output_ptr = nullptr;  // For updating auto mode status
SKOutputFloat* target_output_ptr = nullptr;     // For updating target rode status
bool commands_allowed = false;              // Commands blocked until SignalK connection stable for 5s
unsigned long connection_stable_time = 0;   // Time when we can allow commands

// Helper function to cleanly disable automatic mode from any source
void disableAutoMode() {
    if (auto_mode_controller && auto_mode_controller->isEnabled()) {
        auto_mode_controller->setEnabled(false);
        auto_mode_controller->setTargetLength(-1.0f);
        if (auto_mode_output_ptr) {
            auto_mode_output_ptr->set_input(0.0f);
        }
        if (target_output_ptr) {
            target_output_ptr->set_input(-1.0f);
        }
        debugD("Automatic mode disabled");
    }
}

// Interrupt Service Routine - Pulse Counter with Direction Sensing
void IRAM_ATTR pulseISR() {
    delayMicroseconds(10);  // Allow direction signal to stabilize
    
    if (digitalRead(PinConfig::DIRECTION)) {
        pulse_count++;  // Chain out
    } else {
        pulse_count--;  // Chain in
        if (pulse_count < 0) {
            pulse_count = 0;  // Prevent negative values
        }
    }
}

// Custom sensor class to read pulse count and convert to meters
class PulseCounter : public FloatSensor {
private:
    uint read_delay_;

public:
    PulseCounter(uint read_delay = 100, String config_path = "")
        : FloatSensor(config_path), read_delay_(read_delay) {
        // Set up repeating task to read pulse count
        event_loop()->onRepeat(read_delay_, [this]() { this->update(); });
    }

    void update() {
        float meters = pulse_count * config_meters_per_pulse;
        this->emit(meters);
        
        // Periodic debug output
        static unsigned long last_debug = 0;
        if (millis() - last_debug > 5000) {
            bool dir_state = digitalRead(PinConfig::DIRECTION);
            debugD("Pulse count: %ld, Direction: %s, Chain: %.2f m", 
                   pulse_count, dir_state ? "OUT" : "IN", meters);
            last_debug = millis();
        }
        
        // Home position detection and auto-reset
        if (home_sensor.isHome()) {
            if (winch_controller.isMovingUp()) {
                winch_controller.stop();
                debugD("Anchor home reached - stopped and reset");
            }
            if (home_sensor.justArrived()) {
                pulse_count = 0;
                debugD("Anchor at home - counter reset");
                // Disable auto mode on home arrival (for auto-home feature)
                disableAutoMode();
            }
        }
        
        // Automatic mode control logic
        if (auto_mode_controller) {
            auto_mode_controller->update(meters);
        }
    }

    // Method to reset the counter
    void reset() {
        pulse_count = 0;
        winch_controller.stop();
        if (auto_mode_controller) {
            auto_mode_controller->setTargetLength(-1.0);
        }
        debugD("Pulse counter reset to 0");
    }

    float get_meters_per_pulse() const {
        return config_meters_per_pulse;
    }
};

void setup()
{
    // Commands blocked by default until SignalK connection is stable
    
    // SAFETY FIRST: Set all active-LOW outputs to inactive state immediately
    // Active-LOW relays: HIGH = inactive, LOW = active
    pinMode(PinConfig::WINCH_UP, OUTPUT);
    pinMode(PinConfig::WINCH_DOWN, OUTPUT);
    pinMode(PinConfig::REMOTE_OUT1, OUTPUT);
    pinMode(PinConfig::REMOTE_OUT2, OUTPUT);
    digitalWrite(PinConfig::WINCH_UP, HIGH);
    digitalWrite(PinConfig::WINCH_DOWN, HIGH);
    digitalWrite(PinConfig::REMOTE_OUT1, HIGH);
    digitalWrite(PinConfig::REMOTE_OUT2, HIGH);
    
    // put your setup code here, to run once:
    SetupLogging();

    // Create the global SensESPApp() object
    SensESPAppBuilder builder;
    updateApPasswordIfDefault(AP_PASSWORD);
    sensesp_app = builder
                      .set_wifi_access_point("bow-controller", AP_PASSWORD)
                      ->set_hostname("bow-controller")
                      ->enable_ota(OTA_PASSWORD)
                      ->get_app();

    // Initialize hardware controllers
    winch_controller.initialize();
    home_sensor.initialize();
    
    // Initialize automatic mode controller
    auto_mode_controller = new AutomaticModeController(winch_controller, home_sensor);
    auto_mode_controller->setTolerance(config_meters_per_pulse * 2.0);
    
    // Initialize remote control
    remote_control = new RemoteControl(winch_controller);
    remote_control->initialize();

    // Configure the pulse input pin and direction pin
    pinMode(PinConfig::PULSE_INPUT, INPUT_PULLUP);
    pinMode(PinConfig::DIRECTION, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PinConfig::PULSE_INPUT), pulseISR, RISING);

    // Add configurable meters per pulse value
    String meters_per_pulse_path = "/anchor/meters_per_pulse";
    auto* meters_per_pulse_config = new NumberConfig(config_meters_per_pulse, meters_per_pulse_path);
    ConfigItem(meters_per_pulse_config)
        ->set_title("Meters per Pulse")
        ->set_description("Distance in meters that each pulse represents")
        ->set_sort_order(100);
    
    // Create the pulse counter sensor (update every 1000ms)
    auto* pulse_counter = new PulseCounter(1000, "/pulse_counter/config");
    
    // Connect to SignalK output for anchor chain length with units
    auto* rode_output = new SKOutputFloat("navigation.anchor.currentRode", "/pulse_counter/sk_path");
    rode_output->set_metadata(new SKMetadata("m"));  // Set units to meters
    pulse_counter->connect_to(rode_output);

    // Add SignalK value listener to reset the counter (self-clearing command)
    auto* reset_listener = new BoolSKListener("navigation.anchor.resetRode");
    auto* reset_output = new SKOutputBool("navigation.anchor.resetRode", "/reset_rode/sk_path");
    reset_output->set_input(false);  // Clear command on boot
    reset_listener->connect_to(new LambdaConsumer<bool>([pulse_counter, reset_output](bool reset_signal) {
        if (!commands_allowed) return;  // Block until connection stable
        if (reset_signal) {
            pulse_counter->reset();
            debugD("Reset command triggered");
            // Clear command immediately to allow retriggering
            reset_output->set_input(false);
        }
    }));

    // Manual Windlass Control: Single path with three states (1=UP, 0=STOP, -1=DOWN)
    // Manual control overrides automatic mode
    auto* manual_control_output = new SKOutputInt("navigation.anchor.manualControlStatus", "/manual_control_status/sk_path");
    manual_control_output->set_input(0);  // Initialize to STOP on boot
    auto* manual_control_listener = new IntSKListener("navigation.anchor.manualControl");
    
    manual_control_listener->connect_to(new LambdaTransform<int, int>([](int command) {
        if (!commands_allowed) return 0;  // Block until connection stable
        // Manual control always overrides automatic mode
        disableAutoMode();
        
        if (command == 1) {
            winch_controller.moveUp();
        } else if (command == -1) {
            winch_controller.moveDown();
        } else {
            winch_controller.stop();
        }
        debugD("Manual control: %s", command == 1 ? "UP" : (command == -1 ? "DOWN" : "STOP"));
        return command;
    }))->connect_to(manual_control_output);

    // Automatic Mode Control: Enable/disable automatic windlass control
    // Using FloatSKListener (value > 0.5 = enable, <= 0.5 = disable)
    auto* auto_mode_output = new SKOutputFloat("navigation.anchor.automaticModeStatus", "/automatic_mode_status/sk_path");
    auto_mode_output_ptr = auto_mode_output;  // Store pointer for global helper function
    
    // Target Rode Length: Arm target for automatic mode
    auto* target_output = new SKOutputFloat("navigation.anchor.targetRodeStatus", "/target_rode_status/sk_path");
    target_output_ptr = target_output;  // Store pointer for global helper function
    target_output->set_metadata(new SKMetadata("m"));  // Set units to meters
    
    // Enable remote control to override automatic mode
    remote_control->setAutoModeController(auto_mode_controller);
    remote_control->setAutoModeOutput(auto_mode_output);

    // Ensure auto mode starts disabled on boot and target is cleared
    disableAutoMode();
    
    auto* auto_mode_listener = new FloatSKListener("navigation.anchor.automaticModeCommand");
    
    auto_mode_listener->connect_to(new LambdaTransform<float, float>([pulse_counter](float value) {
        if (!commands_allowed) return value;  // Block until connection stable
        bool enable = (value > 0.5);
        
        if (enable != auto_mode_controller->isEnabled()) {
            if (enable) {
                auto_mode_controller->setEnabled(true);
                debugD("Automatic mode ENABLED");
                if (auto_mode_controller->getTargetLength() >= 0) {
                    float current = pulse_count * pulse_counter->get_meters_per_pulse();
                    debugD("Target armed: %.2f m, current: %.2f m", 
                           auto_mode_controller->getTargetLength(), current);
                    auto_mode_controller->update(current);
                }
            } else {
                debugD("Automatic mode DISABLED");
                disableAutoMode();
            }
        }
        return value;
    }))->connect_to(auto_mode_output);

    auto* target_listener = new FloatSKListener("navigation.anchor.targetRodeCommand");
    
    target_listener->connect_to(new LambdaTransform<float, float>([pulse_counter](float target) {
        if (!commands_allowed) return target;  // Block until connection stable
        // Accept any valid target (including re-sending same value)
        if (target >= 0) {
            auto_mode_controller->setTargetLength(target);
            float current = pulse_count * pulse_counter->get_meters_per_pulse();
            
            debugD("Target armed: %.2f m (current: %.2f m)", target, current);
            
            // Start winch immediately if automatic mode already enabled
            if (auto_mode_controller->isEnabled()) {
                auto_mode_controller->update(current);
            }
        }
        return target;
    }))->connect_to(target_output);

    // Home Command: Arm target to 0.0m (auto-home) - self-clearing
    auto* home_listener = new BoolSKListener("navigation.anchor.homeCommand");
    auto* home_output = new SKOutputBool("navigation.anchor.homeCommand", "/home_command/sk_path");
    home_output->set_input(false);  // Clear command on boot
    home_listener->connect_to(new LambdaConsumer<bool>([home_output](bool go_home) {
        if (!commands_allowed) return;  // Block until connection stable
        if (go_home) {
            if (winch_controller.isActive() && !auto_mode_controller->isEnabled()) {
                debugD("Home command blocked - manual control active");
            } else {
                auto_mode_controller->setTargetLength(0.0f);
                target_output_ptr->set_input(0.0f);
                debugD("Home command armed: target set to 0.0 m");
            }
            // Clear command immediately to allow retriggering
            home_output->set_input(false);
        }
    }));

    // Monitor SignalK connection state - check every 100ms for fast response
    sensesp_app->get_event_loop()->onRepeat(100, []() {
        static bool was_connected = false;
        auto ws_client = sensesp_app->get_ws_client();
        bool is_connected = ws_client ? ws_client->is_connected() : false;
        
        if (was_connected && !is_connected) {
            // Connection lost - immediately stop all automatic operations and block commands
            debugD("SignalK connection lost - stopping automatic operations");
            disableAutoMode();
            winch_controller.stop();
            commands_allowed = false;
            connection_stable_time = 0;
        } else if (!was_connected && is_connected) {
            // Connection established - wait 5 seconds before allowing commands
            connection_stable_time = millis() + 5000;
            commands_allowed = false;
            debugD("SignalK connected - commands blocked for 5 seconds");
        } else if (is_connected && !commands_allowed && connection_stable_time > 0 && millis() >= connection_stable_time) {
            // Connection has been stable for 5 seconds - allow commands
            commands_allowed = true;
            debugD("SignalK connection stable - commands now allowed");
        }
        was_connected = is_connected;
    });

    debugD("=== Anchor Chain Counter System ===");
    debugD("Pulse input: GPIO %d, Direction: GPIO %d", PinConfig::PULSE_INPUT, PinConfig::DIRECTION);
    debugD("Winch control: UP=GPIO %d, DOWN=GPIO %d", PinConfig::WINCH_UP, PinConfig::WINCH_DOWN);
    debugD("Home sensor: GPIO %d", PinConfig::ANCHOR_HOME);
    debugD("Remote outputs: OUT1=GPIO %d, OUT2=GPIO %d", PinConfig::REMOTE_OUT1, PinConfig::REMOTE_OUT2);
    debugD("Mode: MANUAL (automatic disabled)");
    debugD("Startup complete - commands blocked until SignalK connection stable");
}

void loop()
{
    // Process physical remote control inputs
    if (remote_control) {
        remote_control->processInputs();
    }
    
    static auto event_loop = sensesp_app->get_event_loop();
    event_loop->tick();
}
