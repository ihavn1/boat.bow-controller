#pragma once

/**
 * @file bow_propeller_controller.h
 * @brief Manages bow propeller (thruster) control with safety checks
 * 
 * This class encapsulates bow propeller control logic, following the same
 * SOLID principles as WinchController:
 * 
 * - Uses dependency injection for loose coupling (motor abstraction)
 * - Single Responsibility: Only manages bow propeller state and relay activation
 * - Open/Closed: Extensible for additional control modes without modification
 * - Liskov Substitution: Works with any motor implementation
 * 
 * SAFETY: Active-LOW relay logic ensures safe default state (HIGH = inactive)
 * on boot, preventing accidental propeller activation.
 * 
 * DESIGN: Prevents simultaneous port/starboard activation through internal
 * logic (writes HIGH to both relays before activating the desired direction).
 */
class BowPropellerMotor;

class BowPropellerController {
public:
    /**
     * @brief Construct bow propeller controller with dependency injection
     * @param motor Reference to bow propeller motor implementation
     */
    explicit BowPropellerController(BowPropellerMotor& motor)
        : motor_(motor) {}

    /**
     * @brief Turn bow propeller to port (left)
     * Activates the port relay while ensuring starboard is inactive.
     */
    void turnPort();

    /**
     * @brief Turn bow propeller to starboard (right)
     * Activates the starboard relay while ensuring port is inactive.
     */
    void turnStarboard();

    /**
     * @brief Stop bow propeller
     * Deactivates both port and starboard relays.
     */
    void stop();

    /// @return true if propeller is currently active (moving in either direction)
    bool isActive() const;

    /// @return true if propeller is currently turning port
    bool isTurningPort() const;

    /// @return true if propeller is currently turning starboard
    bool isTurningStarboard() const;

private:
    BowPropellerMotor& motor_;  ///< Motor control implementation
};
