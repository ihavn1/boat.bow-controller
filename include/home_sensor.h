#pragma once

#include "interfaces/ISensor.h"

/**
 * @file home_sensor.h
 * @brief Semantic wrapper for home position sensor
 * 
 * This class provides business-logic semantics around a generic sensor.
 * It depends on ISensor interface, allowing any sensor implementation.
 * 
 * DESIGN PRINCIPLE: Composition over Inheritance
 * - Uses ISensor for hardware abstraction
 * - Provides domain-specific methods (isHome, justArrived, justLeft)
 * - Testable with mock sensors
 */
class HomeSensor {
public:
    /**
     * @brief Construct home sensor wrapper
     * @param sensor Reference to sensor implementation (e.g., ESP32Sensor<PIN>)
     */
    HomeSensor(ISensor& sensor) : sensor_(sensor) {}

    /**
     * @brief Check if anchor is currently at home position
     * @return true if anchor is at home
     */
    bool isHome() const;

    /**
     * @brief Detect if anchor just arrived at home
     * @return true if anchor just transitioned to home position
     */
    bool justArrived();

    /**
     * @brief Detect if anchor just left home position
     * @return true if anchor just left home position
     */
    bool justLeft();

private:
    ISensor& sensor_;  ///< Underlying sensor implementation
};
