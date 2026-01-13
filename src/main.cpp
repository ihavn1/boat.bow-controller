#include <memory>
#include "sensesp_app_builder.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/ui/config_item.h"
#include "sensesp/ui/ui_controls.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_value_listener.h"
#include "sensesp/transforms/lambda_transform.h"
#include "sensesp/system/valueconsumer.h"

#include "pin_config.h"
#include "winch_controller.h"
#include "home_sensor.h"
#include "automatic_mode_controller.h"
#include "remote_control.h"

using namespace sensesp;

// Global system components
WinchController winch_controller;
HomeSensor home_sensor;
AutomaticModeController* auto_mode_controller = nullptr;
RemoteControl* remote_control = nullptr;

// Global Variables
volatile long pulse_count = 0;              // Bidirectional pulse counter (ISR access)
float config_meters_per_pulse = 0.01;       // Configurable conversion factor
SKOutputFloat* auto_mode_output_ptr = nullptr;  // For updating auto mode status

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
    // put your setup code here, to run once:
    SetupLogging();

    // Create the global SensESPApp() object
    SensESPAppBuilder builder;
    sensesp_app = builder
                      .set_hostname("bow-sensors")
                      ->get_app();

    // Initialize hardware controllers
    winch_controller.initialize();
    home_sensor.initialize();
    
    // Initialize automatic mode controller
    auto_mode_controller = new AutomaticModeController(winch_controller);
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

    // Add SignalK value listener to reset the counter (receives PUT via delta)
    auto* reset_listener = new BoolSKListener("navigation.anchor.resetRode");
    reset_listener->connect_to(new LambdaConsumer<bool>([pulse_counter](bool reset_signal) {
        if (reset_signal) {
            pulse_counter->reset();
        }
    }));

    // Manual Windlass Control: Single path with three states (1=UP, 0=STOP, -1=DOWN)
    auto* manual_control_output = new SKOutputInt("navigation.anchor.manualControlStatus", "/manual_control_status/sk_path");
    auto* manual_control_listener = new IntSKListener("navigation.anchor.manualControl");
    
    manual_control_listener->connect_to(new LambdaTransform<int, int>([](int command) {
        if (!auto_mode_controller->isEnabled()) {
            if (command == 1) {
                winch_controller.moveUp();
            } else if (command == -1) {
                winch_controller.moveDown();
            } else {
                winch_controller.stop();
            }
            debugD("Manual control: %s", command == 1 ? "UP" : (command == -1 ? "DOWN" : "STOP"));
        } else {
            debugD("Manual control blocked - automatic mode active");
        }
        return command;
    }))->connect_to(manual_control_output);

    // Automatic Mode Control: Enable/disable automatic windlass control
    // Using FloatSKListener (value > 0.5 = enable, <= 0.5 = disable)
    auto* auto_mode_output = new SKOutputFloat("navigation.anchor.automaticModeStatus", "/automatic_mode_status/sk_path");
    
    // Set callback to update SignalK when target is reached
    auto_mode_controller->setOnTargetReachedCallback([auto_mode_output]() {
        if (auto_mode_output) {
            auto_mode_output->set_input(0.0);
        }
    });
    
    auto* auto_mode_listener = new FloatSKListener("navigation.anchor.automaticModeCommand");
    
    auto_mode_listener->connect_to(new LambdaTransform<float, float>([pulse_counter](float value) {
        bool enable = (value > 0.5);
        auto_mode_controller->setEnabled(enable);
        
        if (enable) {
            debugD("Automatic mode ENABLED");
            // If target already armed, trigger immediate position update
            if (auto_mode_controller->getTargetLength() >= 0) {
                float current = pulse_count * pulse_counter->get_meters_per_pulse();
                debugD("Target armed: %.2f m, current: %.2f m", 
                       auto_mode_controller->getTargetLength(), current);
                auto_mode_controller->update(current);
            }
        } else {
            debugD("Automatic mode DISABLED"tomatic mode DISABLED");
            stopWinch();
        }
        return value;
    }))->connect_to(auto_mode_output);

    // Target Rode Length: Arm target for automatic mode
    auto* target_output = new SKOutputFloat("navigation.anchor.targetRodeStatus", "/target_rode_status/sk_path");
    target_output->set_metadata(new SKMetadata("m"));  // Set units to meters
    auto* target_listener = new FloatSKListener("navigation.anchor.targetRodeCommand");
    
    targauto_mode_controller->setTargetLength(target);
        float current = pulse_count * pulse_counter->get_meters_per_pulse();
        
        debugD("Target armed: %.2f m (current: %.2f m)", target, current);
        
        // Start winch immediately if automatic mode already enabled
        if (auto_mode_controller->isEnabled() && target >= 0) {
            auto_mode_controller->update(current);
        }
        
        return target;
    }))->connect_to(target_output);

    debugD("=== Anchor Chain Counter System ===");
    debugD("Pulse input: GPIO %d, Direction: GPIO %d", PinConfig::PULSE_INPUT, PinConfig::DIRECTION);
    debugD("Winch control: UP=GPIO %d, DOWN=GPIO %d", PinConfig::WINCH_UP, PinConfig::WINCH_DOWN);
    debugD("Home sensor: GPIO %d", PinConfig::ANCHOR_HOME);
    debugD("Mode: MANUAL (automatic disabled)");
}

void loop()
{
    // Process physical remote control inputs
    if (remote_control) {
        remote_control->processInputs(auto_mode_controller->isEnabled());
    }
    
    handleManualInputs(); // Check for physical remote control presses
    static auto event_loop = sensesp_app->get_event_loop();
    event_loop->tick();
}
