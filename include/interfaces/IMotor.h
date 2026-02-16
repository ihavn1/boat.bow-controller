#pragma once

/**
 * @file IMotor.h
 * @brief Abstract interface for motor control
 * 
 * This interface defines the contract for any motor implementation.
 * Concrete implementations (e.g., ESP32Motor) handle the actual hardware details.
 * 
 * DESIGN PRINCIPLE: Dependency Inversion
 * - High-level code (WinchController) depends on this abstraction
 * - Low-level code (ESP32Motor) implements this abstraction
 * - Dependencies point toward the abstraction, not vice versa
 */
class IMotor {
public:
    /// Movement direction enumeration
    enum class Direction {
        STOPPED,  ///< Motor is not moving
        UP,       ///< Motor moving up (retrieve)
        DOWN      ///< Motor moving down (deploy)
    };

    virtual ~IMotor() = default;

    /**
     * @brief Activate motor in UP direction
     */
    virtual void moveUp() = 0;

    /**
     * @brief Activate motor in DOWN direction
     */
    virtual void moveDown() = 0;

    /**
     * @brief Stop motor movement
     */
    virtual void stop() = 0;

    /**
     * @brief Get current motor state
     * @return true if motor is currently active (moving)
     */
    virtual bool isActive() const = 0;

    /**
     * @brief Get current movement direction
     * @return Current direction (STOPPED, UP, or DOWN)
     */
    virtual Direction getCurrentDirection() const = 0;

    /**
     * @brief Check if motor is moving up
     * @return true if actively moving up
     */
    virtual bool isMovingUp() const = 0;

    /**
     * @brief Check if motor is moving down
     * @return true if actively moving down
     */
    virtual bool isMovingDown() const = 0;
};
