#pragma once

#include "services/StateManager.h"
#include "winch_controller.h"

/**
 * @file EmergencyStopService.h
 * @brief Service for managing emergency stop functionality
 * 
 * This service handles emergency stop logic:
 * - Activating/deactivating emergency stop
 * - Disabling conflicting control modes (auto mode, manual mode)
 * - Stopping all outputs
 * - Providing callbacks when state changes
 * 
 * DESIGN PRINCIPLE: Single Responsibility
 * - Only responsible for emergency stop state and side effects
 * - Updates StateManager (other services react to state changes)
 * - Can be tested independently with mock StateManager
 */
class EmergencyStopService {
public:
    /**
     * @brief Callback function type for emergency stop state changes
     * Signature: void callback(bool is_active, const char* reason)
     */
    using StateChangeCallback = void(*)(bool, const char*);

    /**
     * @brief Construct emergency stop service
     * @param state_manager Reference to state manager
     * @param winch_controller Reference to winch controller
     */
    EmergencyStopService(StateManager& state_manager,
                        WinchController& winch_controller)
        : state_manager_(state_manager),
          winch_controller_(winch_controller),
          state_change_callback_(nullptr) {}

    /**
     * @brief Activate or deactivate emergency stop
     * @param active true to activate, false to deactivate
     * @param reason String describing why emergency stop changed (for logging)
     */
    void setActive(bool active, const char* reason = "");

    /**
     * @brief Check if emergency stop is currently active
     * @return true if active
     */
    bool isActive() const {
        return state_manager_.isEmergencyStopActive();
    }

    /**
     * @brief Register callback for state change notifications
     * @param callback Function to call when emergency stop state changes
     */
    void onStateChange(StateChangeCallback callback) {
        state_change_callback_ = callback;
    }

private:
    StateManager& state_manager_;           ///< State holder (for emergency stop flag)
    WinchController& winch_controller_;     ///< Winch controller (to stop winch)
    StateChangeCallback state_change_callback_;  ///< Callback for state changes
};
