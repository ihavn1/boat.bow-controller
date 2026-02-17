#pragma once

#include <Arduino.h>
#include "pin_config.h"
#include "winch_controller.h"
#include "automatic_mode_controller.h"
#include "bow_propeller_controller.h"
#include "services/StateManager.h"
#include "sensesp/signalk/signalk_output.h"

using namespace sensesp;

/**
 * @file remote_control.h
 * @brief Handles physical remote control inputs for winch and bow propeller
 * 
 * This class processes inputs from a physical remote control (wired buttons).
 * The remote operates in a "deadman switch" mode for all functions:
 * - Control runs only while a button is held down
 * - Control stops immediately when button is released
 * - Remote overrides automatic mode (disables it immediately on button press)
 * 
 * Button Mapping:
 * - GPIO 12 (REMOTE_UP): Winch UP
 * - GPIO 13 (REMOTE_DOWN): Winch DOWN
 * - GPIO 15 (REMOTE_FUNC3): Bow propeller PORT
 * - GPIO 16 (REMOTE_FUNC4): Bow propeller STARBOARD
 * 
 * Hardware: Remote buttons are active-HIGH (read HIGH when pressed)
 */
class RemoteControl {
public:
    /**
     * @brief Construct remote control handler
     * @param state_manager Reference to state manager
     * @param winch Reference to winch controller (dependency injection)
     * @param auto_mode_controller Pointer to automatic mode controller (can be nullptr)
     * @param auto_mode_output_ptr Pointer to auto mode SignalK output (can be nullptr)
     */
    RemoteControl(StateManager& state_manager,
                  AnchorWinchController& winch, 
                  AutomaticModeController* auto_mode_controller = nullptr,
                  SKOutputFloat* auto_mode_output_ptr = nullptr);

    /**
     * @brief Initialize remote control GPIO pins
     * Sets all remote input pins to INPUT mode and output pins to OUTPUT (inactive HIGH)
     */
    void initialize();

    /**
     * @brief Process remote control inputs (call every loop iteration)
     * @return true if remote is actively controlling the winch, false if not
     * 
     * Behavior (deadman switch):
     * - If UP button pressed: disable auto mode, move winch up, return true
     * - If DOWN button pressed: disable auto mode, move winch down, return true
     * - If neither pressed and remote was controlling: stop winch
     * - If neither pressed and remote was NOT controlling (auto mode active): do nothing
     */
    bool processInputs();

    /**
     * @brief Set the automatic mode controller pointer
     * @param auto_mode_controller Pointer to automatic mode controller
     */
    void setAutoModeController(AutomaticModeController* auto_mode_controller);

    /**
     * @brief Set the auto mode SignalK output pointer
     * @param auto_mode_output_ptr Pointer to auto mode SignalK output
     */
    void setAutoModeOutput(SKOutputFloat* auto_mode_output_ptr);

    /**
     * @brief Set the bow propeller controller pointer
     * @param bow_propeller_controller Pointer to bow propeller controller
     */
    void setBowPropellerController(BowPropellerController* bow_propeller_controller);

private:
    StateManager& state_manager_;  ///< Reference to state manager
    AnchorWinchController& winch_;  ///< Reference to anchor winch controller
    AutomaticModeController* auto_mode_controller_;  ///< Pointer to auto mode controller
    SKOutputFloat* auto_mode_output_ptr_;  ///< Pointer to auto mode SignalK output
    BowPropellerController* bow_propeller_controller_;  ///< Pointer to bow propeller controller
    bool remote_active_ = false;  ///< True if remote is currently controlling the winch
    bool prev_button_active_ = false;  ///< Previous combined button state for edge detection
    unsigned long last_press_ms_ = 0;  ///< Time of last button press
    unsigned long press_start_ms_ = 0;  ///< Time when current press started
    bool long_press_fired_ = false;  ///< True if long-press action already triggered
};

