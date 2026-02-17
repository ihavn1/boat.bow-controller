#pragma once

#include "../pin_config.h"

/**
 * @file ESP32BowPropellerMotor.h
 * @brief ESP32 GPIO implementation for bow propeller (thruster) control
 * 
 * Manages two independent relay outputs:
 * - GPIO 4 (PinConfig::BOW_PORT): Port thruster relay
 * - GPIO 5 (PinConfig::BOW_STARBOARD): Starboard thruster relay
 * 
 * Both relays use active-LOW logic:
 * - Write LOW to activate relay
 * - Write HIGH to deactivate relay (safe default state)
 * 
 * DESIGN PRINCIPLE: Dependency Inversion
 * - This concrete implementation handles hardware details
 * - Higher-level code (BowPropellerController) depends on abstraction
 * - Can be replaced with mock implementation for testing
 */
class BowPropellerMotor {
public:
    /// Direction enumeration for bow propeller
    enum class Direction {
        STOPPED,    ///< Propeller not moving
        PORT,       ///< Turning to port (left)
        STARBOARD   ///< Turning to starboard (right)
    };

    /**
     * @brief Initialize GPIO pins for bow propeller relays
     * Sets pins to OUTPUT mode and ensures both relays start in inactive state.
     * Must be called during setup() before using propeller control.
     */
    void initialize();

    /**
     * @brief Activate port thruster relay
     * Ensures starboard is inactive before activating port.
     */
    void turnPort();

    /**
     * @brief Activate starboard thruster relay
     * Ensures port is inactive before activating starboard.
     */
    void turnStarboard();

    /**
     * @brief Deactivate both thruster relays
     */
    void stop();

    /**
     * @brief Get whether propeller is currently active
     * @return true if either port or starboard is active
     */
    bool isActive() const;

    /**
     * @brief Get current propeller direction
     * @return Current direction (STOPPED, PORT, or STARBOARD)
     */
    Direction getCurrentDirection() const;

    /**
     * @brief Check if propeller is currently turning port
     * @return true if port is active
     */
    bool isTurningPort() const;

    /**
     * @brief Check if propeller is currently turning starboard
     * @return true if starboard is active
     */
    bool isTurningStarboard() const;

private:
    bool active_ = false;                           ///< True if propeller is currently active
    Direction current_direction_ = Direction::STOPPED;  ///< Current turning direction
    unsigned long last_stop_log_ms_ = 0;            ///< Throttle logging

    /**
     * @brief Throttled logging of propeller stop events
     */
    void logStopThrottled();
};
