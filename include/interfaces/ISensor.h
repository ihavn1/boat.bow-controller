#pragma once

/**
 * @file ISensor.h
 * @brief Abstract interface for sensor reading
 * 
 * This interface defines the contract for any sensor implementation.
 * Concrete implementations (e.g., ESP32Sensor) handle the actual hardware details.
 * 
 * DESIGN PRINCIPLE: Dependency Inversion
 * - High-level code (HomeSensor, WinchController) depends on this abstraction
 * - Low-level code (ESP32Sensor) implements this abstraction
 * - Enables testing with mock sensors without hardware
 */
class ISensor {
public:
    virtual ~ISensor() = default;

    /**
     * @brief Read current sensor state
     * @return true if sensor is active (e.g., anchor is at home for home sensor)
     * @note Sensor logic is implementation-specific (may be active-LOW or active-HIGH)
     */
    virtual bool isActive() const = 0;

    /**
     * @brief Detect if sensor just transitioned to active state
     * @return true if sensor became active since last call to update()
     * @note Must call update() periodically to track state changes
     */
    virtual bool justActivated() = 0;

    /**
     * @brief Detect if sensor just transitioned to inactive state
     * @return true if sensor became inactive since last call to update()
     * @note Must call update() periodically to track state changes
     */
    virtual bool justDeactivated() = 0;

    /**
     * @brief Update sensor state tracking for edge detection
     * @note Call this periodically (every loop iteration) to detect state changes
     */
    virtual void update() = 0;
};
