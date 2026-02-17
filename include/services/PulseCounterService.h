#pragma once

#include "services/StateManager.h"
#include "winch_controller.h"
#include "home_sensor.h"

/**
 * @file PulseCounterService.h
 * @brief Service for managing pulse counting and rode length calculation
 * 
 * This service handles:
 * - Reading pulse count from ISR
 * - Converting pulses to rode length
 * - Detecting when anchor reaches home
 * - Updating state through StateManager
 * 
 * DESIGN PRINCIPLE: Single Responsibility
 * - Only responsible for pulse → length conversion and home detection
 * - Updates state manager, which other services react to
 * - Decoupled from GUI, SignalK, or other concerns
 */
class PulseCounterService {
public:
    /**
     * @brief Construct pulse counter service
     * @param state_manager Reference to state manager (for state updates)
     * @param winch_controller Reference to anchor winch controller
     * @param home_sensor Reference to home sensor
     * @param read_delay_ms Delay between pulse count reads (milliseconds)
     */
    PulseCounterService(StateManager& state_manager,
                        AnchorWinchController& winch_controller,
                        HomeSensor& home_sensor,
                        unsigned int read_delay_ms = 100)
        : state_manager_(state_manager),
          winch_controller_(winch_controller),
          home_sensor_(home_sensor),
          read_delay_ms_(read_delay_ms) {}

    /**
     * @brief Initialize the pulse counter service
     * Must be called from setup() to set up the recurring update task
     */
    void initialize();

    /**
     * @brief Update pulse count and rode length (call periodically)
     * Handles home sensor logic and auto-home mode
     */
    void update();

    /**
     * @brief Get current rode length
     * @return Length in meters
     */
    float getRodeLength() const {
        return state_manager_.getRodeLength();
    }

private:
    StateManager& state_manager_;             ///< State holder (reads/writes state)
    AnchorWinchController& winch_controller_;       ///< Anchor winch controller (to stop on home)
    HomeSensor& home_sensor_;                 ///< Home sensor (to detect arrival)
    unsigned int read_delay_ms_;                      ///< Update interval in milliseconds
    unsigned long last_debug_ms_ = 0;         ///< Throttle debug output
};
