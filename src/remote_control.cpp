#include "remote_control.h"

RemoteControl::RemoteControl(StateManager& state_manager,
                             WinchController& winch,
                             AutomaticModeController* auto_mode_controller,
                             SKOutputFloat* auto_mode_output_ptr)
    : state_manager_(state_manager),
      winch_(winch),
      auto_mode_controller_(auto_mode_controller),
      auto_mode_output_ptr_(auto_mode_output_ptr) {}

void RemoteControl::initialize() {
    pinMode(PinConfig::REMOTE_UP, INPUT);
    pinMode(PinConfig::REMOTE_DOWN, INPUT);
    pinMode(PinConfig::REMOTE_FUNC3, INPUT);
    pinMode(PinConfig::REMOTE_FUNC4, INPUT);

    // Initialize spare outputs (inactive HIGH for active-LOW outputs)
    pinMode(PinConfig::REMOTE_OUT1, OUTPUT);
    pinMode(PinConfig::REMOTE_OUT2, OUTPUT);
    digitalWrite(PinConfig::REMOTE_OUT1, HIGH);
    digitalWrite(PinConfig::REMOTE_OUT2, HIGH);
}

bool RemoteControl::processInputs() {
    bool up_pressed = digitalRead(PinConfig::REMOTE_UP) == HIGH;
    bool down_pressed = digitalRead(PinConfig::REMOTE_DOWN) == HIGH;
    bool func3_pressed = digitalRead(PinConfig::REMOTE_FUNC3) == HIGH;
    bool func4_pressed = digitalRead(PinConfig::REMOTE_FUNC4) == HIGH;
    bool any_button_active = up_pressed || down_pressed || func3_pressed || func4_pressed;

    const unsigned long kDoublePressMs = 800;
    const unsigned long kLongPressMs = 2000;
    unsigned long now_ms = millis();

    // Detect rising edge for double-press
    if (any_button_active && !prev_button_active_) {
        if (last_press_ms_ > 0 && (now_ms - last_press_ms_ <= kDoublePressMs)) {
            // Double-press detected: activate emergency stop
            state_manager_.setEmergencyStopActive(true);
        }
        last_press_ms_ = now_ms;
        press_start_ms_ = now_ms;
        long_press_fired_ = false;
    }

    // Long-press to clear emergency stop
    if (any_button_active && state_manager_.isEmergencyStopActive() && !long_press_fired_ && press_start_ms_ > 0 &&
        now_ms - press_start_ms_ >= kLongPressMs) {
        state_manager_.setEmergencyStopActive(false);
        long_press_fired_ = true;
    }

    if (!any_button_active && prev_button_active_) {
        press_start_ms_ = 0;
        long_press_fired_ = false;
    }

    prev_button_active_ = any_button_active;

    if (state_manager_.isEmergencyStopActive()) {
        if (remote_active_) {
            winch_.stop();
            remote_active_ = false;
        }
        return false;
    }

    // Remote button press overrides automatic mode
    if ((up_pressed || down_pressed) && auto_mode_controller_) {
        if (auto_mode_controller_->isEnabled()) {
            auto_mode_controller_->setEnabled(false);
            state_manager_.setAutoModeEnabled(false);
            if (auto_mode_output_ptr_) {
                auto_mode_output_ptr_->set_input(0.0f);
            }
        }
    }

    if (up_pressed) {
        winch_.moveUp();
        remote_active_ = true;
        return true;
    } else if (down_pressed) {
        winch_.moveDown();
        remote_active_ = true;
        return true;
    } else if (remote_active_) {
        // Button released - only stop if remote was actively controlling
        winch_.stop();
        remote_active_ = false;
        return true;
    }
    // Neither button pressed and remote was not controlling - do nothing
    return false;
}

void RemoteControl::setAutoModeController(AutomaticModeController* auto_mode_controller) {
    auto_mode_controller_ = auto_mode_controller;
}

void RemoteControl::setAutoModeOutput(SKOutputFloat* auto_mode_output_ptr) {
    auto_mode_output_ptr_ = auto_mode_output_ptr;
}
