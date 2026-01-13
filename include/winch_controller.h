#pragma once

#include <Arduino.h>
#include "pin_config.h"

/**
 * @file winch_controller.h
 * @brief Manages winch motor control with built-in safety checks
 * 
 * This class encapsulates all winch control logic, including:
 * - Active-LOW relay control (safe default state)
 * - Home sensor safety blocking (prevents over-retrieval)
 * - State tracking (direction, active status)
 * 
 * SAFETY: Winch outputs use active-LOW logic. Pins default to HIGH (inactive)
 * on boot, ensuring the winch cannot start accidentally.
 */
class WinchController {
public:
    /// Winch movement direction states
    enum class Direction {
        STOPPED,  ///< Winch is not moving
        UP,       ///< Winch is retrieving (pulling chain in)
        DOWN      ///< Winch is deploying (letting chain out)
    };

    WinchController() : active_(false), current_direction_(Direction::STOPPED) {}

    /**
     * @brief Initialize GPIO pins for winch control
     * Sets pins to OUTPUT mode and ensures winch starts in stopped state
     */
    void initialize() {
        pinMode(PinConfig::WINCH_UP, OUTPUT);
        pinMode(PinConfig::WINCH_DOWN, OUTPUT);
        stop();
    }

    /**
     * @brief Move winch UP (retrieve chain)
     * @note Automatically blocks if anchor is already at home position
     */
    void moveUp() {
        if (isAtHome()) {
            debugD("Anchor already home - cannot retrieve further");
            stop();
            return;
        }
        digitalWrite(PinConfig::WINCH_DOWN, HIGH);
        digitalWrite(PinConfig::WINCH_UP, LOW);
        active_ = true;
        current_direction_ = Direction::UP;
        debugD("Winch UP activated");
    }

    /**
     * @brief Move winch DOWN (deploy chain)
     */
    void moveDown() {
        digitalWrite(PinConfig::WINCH_UP, HIGH);
        digitalWrite(PinConfig::WINCH_DOWN, LOW);
        active_ = true;
        current_direction_ = Direction::DOWN;
        debugD("Winch DOWN activated");
    }

    /**
     * @brief Stop winch movement
     * Sets both outputs to inactive (HIGH) state
     */
    void stop() {
        digitalWrite(PinConfig::WINCH_UP, HIGH);
        digitalWrite(PinConfig::WINCH_DOWN, HIGH);
        active_ = false;
        current_direction_ = Direction::STOPPED;
        debugD("Winch stopped");
    }

    /// @return true if winch is currently active
    bool isActive() const { return active_; }
    
    /// @return Current movement direction
    Direction getCurrentDirection() const { return current_direction_; }
    
    /// @return true if winch is currently moving up
    bool isMovingUp() const { 
        return active_ && digitalRead(PinConfig::WINCH_UP) == LOW; 
    }
    
    /// @return true if winch is currently moving down
    bool isMovingDown() const { 
        return active_ && digitalRead(PinConfig::WINCH_DOWN) == LOW; 
    }

private:
    bool active_;               ///< True if winch is currently moving
    Direction current_direction_; ///< Current movement direction

    /**
     * @brief Check if anchor is at home position
     * @return true if home sensor reads LOW (anchor at home)
     */
    bool isAtHome() const {
        return digitalRead(PinConfig::ANCHOR_HOME) == LOW;
    }
};
