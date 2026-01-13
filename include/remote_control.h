#pragma once

#include <Arduino.h>
#include "pin_config.h"
#include "winch_controller.h"

/**
 * @file remote_control.h
 * @brief Handles physical remote control inputs for manual winch operation
 * 
 * This class processes inputs from a physical remote control (wired buttons).
 * The remote operates in a "deadman switch" mode:
 * - Winch runs only while a button is held down
 * - Winch stops immediately when button is released
 * - Remote is blocked during automatic mode operation
 * 
 * Hardware: Remote buttons are active-HIGH (read HIGH when pressed)
 */
class RemoteControl {
public:
    /**
     * @brief Construct remote control handler
     * @param winch Reference to winch controller (dependency injection)
     */
    RemoteControl(WinchController& winch) : winch_(winch) {}

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
     * @param automatic_mode_active true if automatic mode is currently active
     * @return true if remote took control, false if blocked or inactive
     * 
     * Behavior:
     * - If automatic mode is active: remote is blocked (returns false)
     * - If UP button pressed: move winch up
     * - If DOWN button pressed: move winch down
     * - If neither pressed: stop winch (deadman switch behavior)
     */
    bool processInputs(bool automatic_mode_active) {
        if (automatic_mode_active) {
            return false;  // Don't allow remote override during automatic mode
        }

        bool up_pressed = digitalRead(PinConfig::REMOTE_UP) == HIGH;
        bool down_pressed = digitalRead(PinConfig::REMOTE_DOWN) == HIGH;

        if (up_pressed) {
            winch_.moveUp();
            return true;
        } else if (down_pressed) {
            winch_.moveDown();
            return true;
        } else {
            // Neither button pressed - stop winch
            winch_.stop();
            return true;
        }
    }

private:
    WinchController& winch_;  ///< Reference to winch controller
};
