#include "services/SignalKService.h"
#include "sensesp/signalk/signalk_value_listener.h"
#include "sensesp/transforms/lambda_transform.h"
#include "sensesp/system/valueconsumer.h"
#include "sensesp_app.h"

using namespace sensesp;

SignalKService::SignalKService(StateManager& state_manager,
                               AnchorWinchController& winch_controller,
                               HomeSensor& home_sensor,
                               AutomaticModeController* auto_mode_controller,
                               EmergencyStopService* emergency_stop_service,
                               PulseCounterService* pulse_counter_service,
                               BowPropellerController* bow_propeller_controller)
    : state_manager_(state_manager),
      winch_controller_(winch_controller),
      home_sensor_(home_sensor),
      auto_mode_controller_(auto_mode_controller),
      emergency_stop_service_(emergency_stop_service),
      pulse_counter_service_(pulse_counter_service),
      bow_propeller_controller_(bow_propeller_controller) {}

void SignalKService::initialize() {
    setupRodeLengthOutput();
    setupEmergencyStopBindings();
    setupManualControlBindings();
    setupAutoModeBindings();
    setupHomeCommandBindings();
    setupBowPropellerBindings();
}

void SignalKService::setupRodeLengthOutput() {
    // Create the rode length output and set up periodic updates from state manager
    rode_output_ = new SKOutputFloat("navigation.anchor.currentRode", "/rode_length_sensor/sk_path");
    rode_output_->set_metadata(new SKMetadata("m"));  // Set units to meters
    rode_output_->set_input(0.0f);  // Initialize to 0
    
    // Periodic emission of rode length every 1000ms
    event_loop()->onRepeat(1000, [this]() { this->updateRodeLength(); });

    // Reset command listener
    auto* reset_listener = new BoolSKListener("navigation.anchor.resetRode");
    reset_output_ = new SKOutputBool("navigation.anchor.resetRode", "/reset_rode/sk_path");
    reset_output_->set_input(false);  // Clear command on boot
    
    reset_listener->connect_to(new LambdaTransform<bool, bool>([this](bool reset_signal) {
        if (state_manager_.isEmergencyStopActive()) return reset_signal;
        if (!state_manager_.areCommandsAllowed()) return reset_signal;  // Block until connection stable
        if (reset_signal) {
            state_manager_.setPulseCount(0);
            state_manager_.setRodeLength(0.0f);
            debugD("Reset command triggered");
            // Clear command immediately to allow retriggering
            reset_output_->set_input(false);
        }
        return reset_signal;
    }));
}

void SignalKService::setupEmergencyStopBindings() {
    auto* emergency_cmd_listener = new BoolSKListener("navigation.bow.ecu.emergencyStopCommand");
    
    // Create ObservableValue for status with automatic SignalK emission
    emergency_stop_status_value_ = new ObservableValue<bool>();
    emergency_stop_status_value_->connect_to(new SKOutputBool(
        "navigation.bow.ecu.emergencyStopStatus",
        "/emergency_stop_status/sk_path"
    ));
    emergency_stop_status_value_->set(false);
    emergency_stop_status_value_->notify();  // Initialize and emit first value
    
    emergency_cmd_listener->connect_to(new LambdaTransform<bool, bool>([this](bool emergency_active) {
        if (!state_manager_.areCommandsAllowed() && !state_manager_.isEmergencyStopActive()) {
            // During startup, force status to false
            if (emergency_stop_status_value_) {
                emergency_stop_status_value_->set(false);
                emergency_stop_status_value_->notify();
            }
            return emergency_active;
        }

        // Process command: delegate to emergency stop service
        if (emergency_stop_service_) {
            emergency_stop_service_->setActive(emergency_active, "signalk");
            
            // Update status value to reflect actual state and emit to SignalK
            if (emergency_stop_status_value_) {
                bool actual_state = emergency_stop_service_->isActive();
                emergency_stop_status_value_->set(actual_state);
                emergency_stop_status_value_->notify();
                debugD("Emergency stop status updated: %s", actual_state ? "ACTIVE" : "CLEARED");
            }
        }
        return emergency_active;
    }))->connect_to(emergency_stop_status_value_);
}

void SignalKService::setupManualControlBindings() {
    // Manual Windlass Control: Single path with three states (1=UP, 0=STOP, -1=DOWN)
    // Manual control overrides automatic mode
    manual_control_output_ = new SKOutputInt("navigation.anchor.manualControlStatus", "/manual_control_status/sk_path");
    manual_control_output_->set_input(0);  // Initialize to STOP on boot
    auto* manual_control_listener = new IntSKListener("navigation.anchor.manualControl");
    
    manual_control_listener->connect_to(new LambdaTransform<int, int>([this](int command) {
        if (state_manager_.isEmergencyStopActive()) return 0;
        if (!state_manager_.areCommandsAllowed()) return 0;  // Block until connection stable
        // Manual control always overrides automatic mode
        if (auto_mode_controller_) {
            auto_mode_controller_->setEnabled(false);
            state_manager_.setAutoModeEnabled(false);
            if (auto_mode_output_) {
                auto_mode_output_->set_input(0.0f);
            }
        }
        
        if (command == 1) {
            winch_controller_.moveUp();
        } else if (command == -1) {
            winch_controller_.moveDown();
        } else {
            winch_controller_.stop();
        }
        debugD("Manual control: %s", command == 1 ? "UP" : (command == -1 ? "DOWN" : "STOP"));
        return command;
    }))->connect_to(manual_control_output_);
}

void SignalKService::setupAutoModeBindings() {
    // Automatic Mode Control: Enable/disable automatic windlass control
    // Using FloatSKListener (value > 0.5 = enable, <= 0.5 = disable)
    auto_mode_output_ = new SKOutputFloat("navigation.anchor.automaticModeStatus", "/automatic_mode_status/sk_path");
    
    // Target Rode Length: Arm target for automatic mode
    target_output_ = new SKOutputFloat("navigation.anchor.targetRodeStatus", "/target_rode_status/sk_path");
    target_output_->set_metadata(new SKMetadata("m"));  // Set units to meters
    
    // Ensure auto mode starts disabled on boot and target is cleared
    if (auto_mode_controller_) {
        auto_mode_controller_->setEnabled(false);
        state_manager_.setAutoModeEnabled(false);
    }
    auto_mode_output_->set_input(0.0f);
    target_output_->set_input(-1.0f);
    
    auto* auto_mode_listener = new FloatSKListener("navigation.anchor.automaticModeCommand");
    
    auto_mode_listener->connect_to(new LambdaTransform<float, float>([this](float value) {
        if (state_manager_.isEmergencyStopActive()) return 0.0f;
        if (!state_manager_.areCommandsAllowed()) return 0.0f;  // Block until connection stable
        bool enable = (value > 0.5);
        
        if (!auto_mode_controller_) return value;
        
        if (enable != auto_mode_controller_->isEnabled()) {
            if (enable) {
                auto_mode_controller_->setEnabled(true);
                state_manager_.setAutoModeEnabled(true);
                debugD("Automatic mode ENABLED");
                if (auto_mode_controller_->getTargetLength() >= 0) {
                    float current = state_manager_.getRodeLength();
                    debugD("Target armed: %.2f m, current: %.2f m", 
                           auto_mode_controller_->getTargetLength(), current);
                    auto_mode_controller_->update(current);
                }
            } else {
                debugD("Automatic mode DISABLED");
                auto_mode_controller_->setEnabled(false);
                state_manager_.setAutoModeEnabled(false);
            }
        }
        return value;
    }))->connect_to(auto_mode_output_);

    auto* target_listener = new FloatSKListener("navigation.anchor.targetRodeCommand");
    
    target_listener->connect_to(new LambdaTransform<float, float>([this](float target) {
        if (state_manager_.isEmergencyStopActive()) return -1.0f;
        if (!state_manager_.areCommandsAllowed()) return target;  // Block until connection stable
        
        if (!auto_mode_controller_) return target;
        
        // Accept any valid target (including re-sending same value)
        if (target >= 0) {
            auto_mode_controller_->setTargetLength(target);
            state_manager_.setAutoModeTarget(target);
            float current = state_manager_.getRodeLength();
            
            debugD("Target armed: %.2f m (current: %.2f m)", target, current);

            // If auto mode was enabled before arming, disable it to enforce arm-then-enable
            if (auto_mode_controller_->isEnabled()) {
                auto_mode_controller_->setEnabled(false);
                state_manager_.setAutoModeEnabled(false);
                auto_mode_output_->set_input(0.0f);
                debugD("Auto mode disabled - target armed requires re-enable");
            }
        }
        return target;
    }))->connect_to(target_output_);
}

void SignalKService::setupHomeCommandBindings() {
    // Home Command: Arm target to 0.0m (auto-home) - self-clearing
    auto* home_listener = new BoolSKListener("navigation.anchor.homeCommand");
    home_command_output_ = new SKOutputBool("navigation.anchor.homeCommand", "/home_command/sk_path");
    home_command_output_->set_input(false);  // Clear command on boot
    
    home_listener->connect_to(new LambdaTransform<bool, bool>([this](bool go_home) {
        if (state_manager_.isEmergencyStopActive()) {
            if (go_home) {
                home_command_output_->set_input(false);
            }
            return go_home;
        }
        if (!state_manager_.areCommandsAllowed()) return go_home;  // Block until connection stable
        
        if (!auto_mode_controller_) return go_home;
        
        if (go_home) {
            if (winch_controller_.isActive() && !auto_mode_controller_->isEnabled()) {
                debugD("Home command blocked - manual control active");
            } else {
                auto_mode_controller_->setTargetLength(0.0f);
                state_manager_.setAutoModeTarget(0.0f);
                target_output_->set_input(0.0f);
                debugD("Home command armed: target set to 0.0 m");
                if (auto_mode_controller_->isEnabled()) {
                    auto_mode_controller_->setEnabled(false);
                    state_manager_.setAutoModeEnabled(false);
                    auto_mode_output_->set_input(0.0f);
                    debugD("Auto mode disabled - home armed requires re-enable");
                }
            }
            // Clear command immediately to allow retriggering
            home_command_output_->set_input(false);
        }
        return go_home;
    }));
}

void SignalKService::updateRodeLength() {
    if (rode_output_) {
        float rode_length = state_manager_.getRodeLength();
        rode_output_->set_input(rode_length);
    }
}

void SignalKService::startConnectionMonitoring() {
    // Monitor SignalK connection state - check every 100ms for fast response
    event_loop()->onRepeat(100, [this]() {
        static bool was_connected = false;
        auto ws_client = sensesp_app->get_ws_client();
        bool is_connected = ws_client ? ws_client->is_connected() : false;
        
        if (was_connected && !is_connected) {
            // Connection lost - immediately stop all automatic operations and block commands
            debugD("SignalK connection lost - stopping automatic operations");
            if (auto_mode_controller_) {
                auto_mode_controller_->setEnabled(false);
                state_manager_.setAutoModeEnabled(false);
            }
            winch_controller_.stop();
            state_manager_.setCommandsAllowed(false);
            connection_stable_time_ = 0;
        } else if (!was_connected && is_connected) {
            // Connection established - wait 5 seconds before allowing commands
            connection_stable_time_ = millis() + 5000;
            state_manager_.setCommandsAllowed(false);
            debugD("SignalK connected - commands blocked for 5 seconds");
        } else if (is_connected && !state_manager_.areCommandsAllowed() && connection_stable_time_ > 0 && millis() >= connection_stable_time_) {
            // Connection has been stable for 5 seconds - allow commands
            state_manager_.setCommandsAllowed(true);
            debugD("SignalK connection stable - commands now allowed");
        }
        was_connected = is_connected;
        
        // Sync emergency stop status back to SignalK (in case it was triggered by physical remote)
        if (emergency_stop_status_value_ && emergency_stop_service_) {
            bool actual_state = emergency_stop_service_->isActive();
            bool current_value = emergency_stop_status_value_->get();
            if (actual_state != current_value) {
                emergency_stop_status_value_->set(actual_state);
                emergency_stop_status_value_->notify();
                debugD("Emergency stop status synced: %s", actual_state ? "ACTIVE" : "CLEARED");
            }
        }
    });
}

void SignalKService::setupBowPropellerBindings() {
    if (!bow_propeller_controller_) {
        debugD("Bow propeller controller not available - skipping SignalK bindings");
        return;
    }
    
    // Bow Propeller Command: Three states (-1=PORT, 0=STOP, 1=STARBOARD)
    bow_propeller_command_output_ = new SKOutputInt("propulsion.bowThruster.command", "/bow_propeller_command/sk_path");
    bow_propeller_command_output_->set_input(0);  // Initialize to STOP on boot
    
    bow_propeller_status_output_ = new SKOutputInt("propulsion.bowThruster.status", "/bow_propeller_status/sk_path");
    bow_propeller_status_output_->set_input(0);  // Initialize to STOP on boot
    
    auto* bow_command_listener = new IntSKListener("propulsion.bowThruster.command");
    
    bow_command_listener->connect_to(new LambdaTransform<int, int>([this](int command) {
        if (state_manager_.isEmergencyStopActive()) return 0;
        if (!state_manager_.areCommandsAllowed()) return 0;  // Block until connection stable
        
        if (command == 1) {
            bow_propeller_controller_->turnStarboard();
            if (bow_propeller_status_output_) {
                bow_propeller_status_output_->set_input(1);
            }
            debugD("Bow propeller command: STARBOARD");
        } else if (command == -1) {
            bow_propeller_controller_->turnPort();
            if (bow_propeller_status_output_) {
                bow_propeller_status_output_->set_input(-1);
            }
            debugD("Bow propeller command: PORT");
        } else {
            bow_propeller_controller_->stop();
            if (bow_propeller_status_output_) {
                bow_propeller_status_output_->set_input(0);
            }
            debugD("Bow propeller command: STOP");
        }
        return command;
    }))->connect_to(bow_propeller_command_output_);
}
