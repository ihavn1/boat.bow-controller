#pragma once

#include "winch_controller.h"
#include "home_sensor.h"

/**
 * @file automatic_mode_controller.h
 * @brief Manages automatic winch positioning to reach target rode length
 * 
 * This controller implements the "arm-and-fire" automatic mode:
 * 1. Target length is "armed" (set but not active)
 * 2. Automatic mode is "fired" (enabled)
 * 3. Controller drives winch to reach target within tolerance
 * 4. Automatically disables when target is reached
 * 
 * The controller uses a simple bang-bang control strategy with configurable
 * tolerance to prevent oscillation around the target.
 */
class AutomaticModeController {
public:
    /**
     * @brief Construct automatic mode controller
     * @param winch Reference to winch controller (dependency injection)
     * @param home_sensor Reference to home sensor for auto-home safety
     */
    AutomaticModeController(WinchController& winch, HomeSensor& home_sensor);

    /**
     * @brief Enable or disable automatic mode
     * @param enabled true to enable automatic control, false to disable
     * @note Disabling stops the winch immediately
     */
    void setEnabled(bool enabled);

    /// @return true if automatic mode is currently enabled
    bool isEnabled() const;

    /**
     * @brief Set target rode length for automatic positioning
     * @param meters Target length in meters (-1 = no target)
     */
    void setTargetLength(float meters);

    /// @return Current target length in meters (-1 if no target set)
    float getTargetLength() const;

    /**
     * @brief Set positioning tolerance
     * @param meters Tolerance in meters (default 0.2m)
     * @note Winch stops when within ±tolerance of target
     */
    void setTolerance(float meters);

    /**
     * @brief Check and clear target reached flag
     * @return true if target was reached since last check
     */
    bool consumeTargetReached();

    /**
     * @brief Update winch position to reach target (call periodically)
     * @param current_length Current rode length in meters
     * 
     * This method implements the control loop:
     * - If target is 0.0 (auto-home): only move up, stop is handled by home sensor
     * - If at target (within tolerance): stop winch, disable automatic mode
     * - If below target: deploy more chain (move down)
     * - If above target: retrieve chain (move up)
     */
    void update(float current_length);

private:
    WinchController& winch_;           ///< Reference to winch controller
    HomeSensor& home_sensor_;          ///< Reference to home sensor
    bool enabled_;                     ///< True if automatic mode is active
    float target_length_;              ///< Target rode length in meters
    float tolerance_;                  ///< Positioning tolerance in meters
    bool target_reached_;              ///< True when target reached since last check
};
