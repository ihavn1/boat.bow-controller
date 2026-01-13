#pragma once

#include <Arduino.h>
#include "pin_config.h"

/**
 * @file home_sensor.h
 * @brief Monitors anchor home position sensor and detects state changes
 * 
 * The home sensor indicates when the anchor is fully retrieved (at home position).
 * This class tracks state changes to detect when the anchor arrives at or leaves home.
 * 
 * Hardware: Sensor is active-LOW (reads LOW when anchor is at home)
 */
class HomeSensor {
public:
    /**
     * @brief Initialize home sensor GPIO pin
     * Sets pin to INPUT_PULLUP mode and reads initial state
     */
    void initialize() {
        pinMode(PinConfig::ANCHOR_HOME, INPUT_PULLUP);
        was_home_ = isHome();
    }

    /**
     * @brief Check if anchor is currently at home position
     * @return true if anchor is at home (sensor reads LOW)
     */
    bool isHome() const {
        return digitalRead(PinConfig::ANCHOR_HOME) == LOW;
    }

    /**
     * @brief Detect if anchor just arrived at home
     * @return true if anchor just transitioned to home position
     * @note Call this repeatedly to track state changes
     */
    bool justArrived() {
        bool current_state = isHome();
        bool just_arrived = current_state && !was_home_;
        was_home_ = current_state;
        return just_arrived;
    }

    /**
     * @brief Detect if anchor just left home position
     * @return true if anchor just left home position
     * @note Call this repeatedly to track state changes
     */
    bool justLeft() {
        bool current_state = isHome();
        bool just_left = !current_state && was_home_;
        was_home_ = current_state;
        return just_left;
    }

private:
    bool was_home_ = false;  ///< Previous home state for edge detection
};
