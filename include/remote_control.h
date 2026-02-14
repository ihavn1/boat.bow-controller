#pragma once

#include <Arduino.h>
#include "pin_config.h"
#include "winch_controller.h"
#include "automatic_mode_controller.h"
#include "sensesp/signalk/signalk_output.h"

using namespace sensesp;

/**
 * @file remote_control.h
 * @brief Handles physical remote control inputs for manual winch operation
 * 
 * This class processes inputs from a physical remote control (wired buttons).
 * The remote operates in a "deadman switch" mode:
 * - Winch runs only while a button is held down
 * - Winch stops immediately when button is released
 * - Remote overrides automatic mode (disables it immediately on button press)
 * 
 * Hardware: Remote buttons are active-HIGH (read HIGH when pressed)
 */
class RemoteControl {
public:
    /**
     * @brief Construct remote control handler
     * @param winch Reference to winch controller (dependency injection)
     * @param auto_mode_controller Pointer to automatic mode controller (can be nullptr)
     * @param auto_mode_output_ptr Pointer to auto mode SignalK output (can be nullptr)
     */
    RemoteControl(WinchController& winch, 
                  AutomaticModeController* auto_mode_controller = nullptr,
                  SKOutputFloat* auto_mode_output_ptr = nullptr) 
        : winch_(winch), 
          auto_mode_controller_(auto_mode_controller),
          auto_mode_output_ptr_(auto_mode_output_ptr) {}

    /**
     * @brief Initialize remote control GPIO pins
     * Sets all remote input pins to INPUT mode and output pins to OUTPUT (inactive HIGH)
     */
    void initialize() {
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

    /**
     * @brief Process remote control inputs (call every loop iteration)
     * @return true if remote is actively controlling the winch, false if not
     * 
     * Behavior (deadman switch):
     * - If UP button pressed: disable auto mode, move winch up, return true
     * - If DOWN button pressed: disable auto mode, move winch down, return true
     * - If neither pressed and remote was controlling: stop winch
     * - If neither pressed and remote was NOT controlling (auto mode active): do nothing
     */
    bool processInputs() {
        bool up_pressed = digitalRead(PinConfig::REMOTE_UP) == HIGH;
        bool down_pressed = digitalRead(PinConfig::REMOTE_DOWN) == HIGH;
        bool button_active = up_pressed || down_pressed;

        // Remote button press overrides automatic mode (declared extern in main.cpp)
        extern void disableAutoMode();
        if (button_active && auto_mode_controller_) {
            if (auto_mode_controller_->isEnabled()) {
                disableAutoMode();
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

    /**
     * @brief Set the automatic mode controller pointer
     * @param auto_mode_controller Pointer to automatic mode controller
     */
    void setAutoModeController(AutomaticModeController* auto_mode_controller) {
        auto_mode_controller_ = auto_mode_controller;
    }

    /**
     * @brief Set the auto mode SignalK output pointer
     * @param auto_mode_output_ptr Pointer to auto mode SignalK output
     */
    void setAutoModeOutput(SKOutputFloat* auto_mode_output_ptr) {
        auto_mode_output_ptr_ = auto_mode_output_ptr;
    }

private:
    WinchController& winch_;  ///< Reference to winch controller
    AutomaticModeController* auto_mode_controller_;  ///< Pointer to auto mode controller
    SKOutputFloat* auto_mode_output_ptr_;  ///< Pointer to auto mode SignalK output
    bool remote_active_ = false;  ///< True if remote is currently controlling the winch
};

