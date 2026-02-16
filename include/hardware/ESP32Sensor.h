#pragma once

#include "Arduino.h"
#include "../interfaces/ISensor.h"
#include "../pin_config.h"

/**
 * @file ESP32Sensor.h
 * @brief ESP32 GPIO implementation of ISensor interface
 * 
 * Concrete implementation for reading sensor states via GPIO pins.
 * Handles edge detection (justActivated/justDeactivated) automatically.
 * 
 * Uses active-LOW logic by default (sensor is active when pin reads LOW).
 * This template can be instantiated with different pins.
 * 
 * Example usage:
 *   ESP32Sensor<PinConfig::ANCHOR_HOME> home_sensor;
 */
template <uint8_t PIN>
class ESP32Sensor : public ISensor {
public:
    /**
     * @brief Initialize GPIO pin for sensor reading
     * Sets pin to INPUT_PULLUP mode and reads initial state.
     * Must be called during setup() before using sensor.
     */
    void initialize() {
        pinMode(PIN, INPUT_PULLUP);
        was_active_ = readState();
    }

    // ISensor implementation
    bool isActive() const override {
        return readState();
    }

    bool justActivated() override {
        bool current = readState();
        bool just_activated = current && !was_active_;
        was_active_ = current;
        return just_activated;
    }

    bool justDeactivated() override {
        bool current = readState();
        bool just_deactivated = !current && was_active_;
        was_active_ = current;
        return just_deactivated;
    }

    void update() override {
        // Edge detection is handled in justActivated/justDeactivated
        // This method exists for compatibility with ISensor interface
        // but the state is automatically updated when querying status
    }

private:
    bool was_active_ = false;   ///< Previous active state for edge detection

    /**
     * @brief Read the actual sensor state from GPIO
     * @return true if sensor is active (pin reads LOW)
     * 
     * ACTIVE-LOW logic: 
     * - LOW (0V) = sensor active
     * - HIGH (3.3V) = sensor inactive
     */
    bool readState() const {
        // This sensor uses active-LOW logic (LOW = active)
        return digitalRead(PIN) == LOW;
    }
};
