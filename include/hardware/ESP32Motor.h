#pragma once

#include "../interfaces/IMotor.h"
#include "../pin_config.h"

/**
 * @file ESP32Motor.h
 * @brief ESP32 GPIO implementation of IMotor interface
 * 
 * Concrete implementation for controlling motor relays via GPIO pins.
 * Uses active-LOW logic: write LOW to activate, HIGH to deactivate.
 * 
 * This implementation is specific to ESP32 with the pin configuration
 * defined in PinConfig. Other platforms would have different implementations
 * but all would implement the IMotor interface.
 */
class ESP32Motor : public IMotor {
public:
    /**
     * @brief Initialize GPIO pins for motor control
     * Sets pins to OUTPUT mode and ensures motor starts in stopped state.
     * Must be called during setup() before using motor control.
     */
    void initialize();

    // IMotor implementation
    void moveUp() override;
    void moveDown() override;
    void stop() override;
    bool isActive() const override;
    Direction getCurrentDirection() const override;
    bool isMovingUp() const override;
    bool isMovingDown() const override;

private:
    bool active_ = false;                    ///< True if motor is currently active
    Direction current_direction_ = Direction::STOPPED;  ///< Current movement direction
    unsigned long last_stop_log_ms_ = 0;     ///< Throttle logging
    
    /**
     * @brief Throttled logging of motor stop events
     */
    void logStopThrottled();
};
