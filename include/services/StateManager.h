#pragma once

/**
 * @file StateManager.h
 * @brief Central state holder for the anchor counter application
 * 
 * This class serves as the single source of truth for all application state.
 * Instead of scattered global variables, all state is managed here.
 * 
 * DESIGN PRINCIPLE: Single Responsibility
 * - Owns and manages all application state
 * - Other components read from and write to this state
 * - State changes are the interface between components
 * 
 * This decouples components from each other - they don't need to know about
 * each other, only about the shared state.
 */
class StateManager {
public:
    // ========== Rope/Chain State ==========
    
    /**
     * @brief Get current pulse count
     * @return Raw pulse count (increments on chain out, decrements on chain in)
     */
    long getPulseCount() const { return pulse_count_; }
    
    /**
     * @brief Set pulse count
     * @param count New pulse count value
     */
    void setPulseCount(long count) { pulse_count_ = count; }
    
    /**
     * @brief Increment pulse count (chain deploying)
     */
    void incrementPulse() { pulse_count_++; }
    
    /**
     * @brief Decrement pulse count (chain retrieving)
     */
    void decrementPulse() {
        pulse_count_--;
        if (pulse_count_ < 0) {
            pulse_count_ = 0;
        }
    }
    
    /**
     * @brief Get current rode length in meters
     * @return Calculated length (pulse_count * meters_per_pulse)
     */
    float getRodeLength() const { return rode_length_; }
    
    /**
     * @brief Set rode length in meters
     * @param length Length in meters
     */
    void setRodeLength(float length) { rode_length_ = length; }
    
    // ========== Configuration ==========
    
    /**
     * @brief Get conversion factor for pulse count to meters
     * @return Meters per pulse
     */
    float getMetersPerPulse() const { return meters_per_pulse_; }
    
    /**
     * @brief Set conversion factor for pulse count to meters
     * @param factor Meters per pulse
     */
    void setMetersPerPulse(float factor) { meters_per_pulse_ = factor; }
    
    // ========== Emergency Stop State ==========
    
    /**
     * @brief Check if emergency stop is active
     * @return true if emergency stop is active
     */
    bool isEmergencyStopActive() const { return emergency_stop_active_; }
    
    /**
     * @brief Set emergency stop state
     * @param active true to activate, false to deactivate
     */
    void setEmergencyStopActive(bool active) { emergency_stop_active_ = active; }
    
    // ========== Automatic Mode State ==========
    
    /**
     * @brief Check if automatic mode is enabled
     * @return true if automatic mode is active
     */
    bool isAutoModeEnabled() const { return auto_mode_enabled_; }
    
    /**
     * @brief Set automatic mode state
     * @param enabled true to enable, false to disable
     */
    void setAutoModeEnabled(bool enabled) { auto_mode_enabled_ = enabled; }
    
    /**
     * @brief Get target rode length for automatic mode
     * @return Target length in meters (-1 if no target)
     */
    float getAutoModeTarget() const { return auto_mode_target_; }
    
    /**
     * @brief Set target rode length for automatic mode
     * @param target Target length in meters (-1 = no target)
     */
    void setAutoModeTarget(float target) { auto_mode_target_ = target; }
    
    // ========== Communication State ==========
    
    /**
     * @brief Check if commands are allowed
     * @return true if SignalK connection is stable and commands are allowed
     */
    bool areCommandsAllowed() const { return commands_allowed_; }
    
    /**
     * @brief Set whether commands are allowed
     * @param allowed true to allow commands
     */
    void setCommandsAllowed(bool allowed) { commands_allowed_ = allowed; }
    
    // ========== Manual Control State ==========
    
    /**
     * @brief Get current manual control command
     * @return 1=UP, 0=STOP, -1=DOWN
     */
    int getManualControl() const { return manual_control_; }
    
    /**
     * @brief Set manual control command
     * @param command 1=UP, 0=STOP, -1=DOWN
     */
    void setManualControl(int command) { manual_control_ = command; }

private:
    // Rope/chain state
    long pulse_count_ = 0;                   ///< Bidirectional pulse counter
    float rode_length_ = 0.0f;               ///< Current rode length in meters
    
    // Configuration
    float meters_per_pulse_ = 0.01f;         ///< Conversion factor: pulses to meters
    
    // Emergency stop state
    bool emergency_stop_active_ = false;    ///< True when emergency stop is active
    
    // Automatic mode state
    bool auto_mode_enabled_ = false;         ///< True when automatic mode is active
    float auto_mode_target_ = -1.0f;         ///< Target rode length (-1 = no target)
    
    // Communication state
    bool commands_allowed_ = false;          ///< True when SignalK connection stable
    
    // Manual control state
    int manual_control_ = 0;                 ///< 1=UP, 0=STOP, -1=DOWN
};
