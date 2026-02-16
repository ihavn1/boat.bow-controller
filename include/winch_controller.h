#pragma once

#include "interfaces/IMotor.h"
#include "interfaces/ISensor.h"

/**
 * @file winch_controller.h
 * @brief Manages winch motor control with built-in safety checks
 * 
 * This class encapsulates all winch control logic, including:
 * - Motor control via IMotor interface (abstracted hardware)
 * - Home sensor safety blocking (prevents over-retrieval)
 * 
 * DESIGN PRINCIPLE: Dependency Injection + Dependency Inversion
 * - Depends on IMotor and ISensor interfaces, not concrete GPIO implementations
 * - Allows testing with mock motors/sensors without hardware
 * - Can be used with any motor/sensor implementation
 * 
 * SAFETY: Motor outputs use active-LOW logic. Pins default to HIGH (inactive)
 * on boot, ensuring the motor cannot start accidentally.
 */
class WinchController {
public:
    /**
     * @brief Construct winch controller with dependency injection
     * @param motor Reference to motor implementation (e.g., ESP32Motor)
     * @param home_sensor Reference to home sensor implementation (e.g., ESP32Sensor<PIN>)
     */
    WinchController(IMotor& motor, ISensor& home_sensor)
        : motor_(motor), home_sensor_(home_sensor) {}

    /**
     * @brief Move winch UP (retrieve chain)
     * @note Automatically blocks if anchor is already at home position
     */
    void moveUp();

    /**
     * @brief Move winch DOWN (deploy chain)
     */
    void moveDown();

    /**
     * @brief Stop winch movement
     */
    void stop();

    /// @return true if motor is currently active
    bool isActive() const;

    /// @return true if motor is currently moving up
    bool isMovingUp() const;

    /// @return true if motor is currently moving down
    bool isMovingDown() const;

private:
    IMotor& motor_;              ///< Motor control implementation
    ISensor& home_sensor_;       ///< Home position sensor implementation
};
