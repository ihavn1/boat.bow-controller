#pragma once

/**
 * @file pin_config.h
 * @brief Centralized GPIO pin configuration for the anchor chain counter system
 * 
 * All hardware pin assignments are defined here as constexpr constants.
 * This makes it easy to reconfigure for different hardware setups.
 */

// GPIO Pin Configuration
struct PinConfig {
    // Sensor inputs
    static constexpr uint8_t PULSE_INPUT = 25;  ///< Pulse input from chain counter sensor
    static constexpr uint8_t DIRECTION = 26;    ///< Direction sensing (HIGH = chain out, LOW = chain in)
    static constexpr uint8_t ANCHOR_HOME = 33;  ///< Anchor home position sensor (LOW = at home)
    
    // Winch outputs (active-LOW: write LOW to activate, HIGH to deactivate)
    static constexpr uint8_t WINCH_UP = 27;     ///< Winch control - UP (retrieve chain)
    static constexpr uint8_t WINCH_DOWN = 14;   ///< Winch control - DOWN (deploy chain)
    
    // Remote control inputs
    static constexpr uint8_t REMOTE_UP = 12;    ///< Physical remote UP button (active HIGH)
    static constexpr uint8_t REMOTE_DOWN = 13;  ///< Physical remote DOWN button (active HIGH)
    static constexpr uint8_t REMOTE_FUNC3 = 15; ///< Spare remote input (future use)
    static constexpr uint8_t REMOTE_FUNC4 = 16; ///< Spare remote input (future use)
    
    // Remote control outputs (active-LOW: write LOW to activate, HIGH to deactivate)
    static constexpr uint8_t REMOTE_OUT1 = 4;   ///< Spare remote output 1
    static constexpr uint8_t REMOTE_OUT2 = 5;   ///< Spare remote output 2
};
