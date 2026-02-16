#pragma once

#include "StateManager.h"
#include "PulseCounterService.h"
#include "EmergencyStopService.h"
#include "hardware/ESP32Motor.h"
#include "hardware/ESP32Sensor.h"
#include "winch_controller.h"
#include "home_sensor.h"
#include "automatic_mode_controller.h"
#include "remote_control.h"
#include "pin_config.h"

// Forward declaration to avoid including SignalKService.h which pulls in sensesp_app_builder.h
class SignalKService;

/**
 * @file BoatAnchorApp.h
 * @brief Main application orchestrator for the anchor chain counter system
 * 
 * Coordinates initialization and management of all system components:
 * - Hardware abstraction (motor, sensors)
 * - Business logic controllers (winch, home, auto mode, remote)
 * - Service layer (state management, emergency stop, pulse counting, SignalK)
 * 
 * Follows the orchestrator pattern to encapsulate system complexity and provide
 * a simple interface for setup and operation.
 */
class BoatAnchorApp {
public:
    /**
     * @brief Construct the application without initializing
     * Call initialize() afterwards in setup()
     */
    BoatAnchorApp();

    /**
     * @brief Initialize all hardware and services
     * Must be called during Arduino setup()
     * Initializes (in order):
     *   1. Hardware (GPIO, pins)
     *   2. Controllers (WinchController, HomeSensor, AutomaticModeController, RemoteControl)
     *   3. Services (EmergencyStopService, PulseCounterService)
     *   4. Pulse counter ISR
     * 
     * Call startSignalK() after SensESP app initialization
     */
    void initialize();

    /**
     * @brief Start SignalK integration
     * Must be called after SensESP initialization in setup()
     * Initializes SignalKService and connection monitoring
     */
    void startSignalK();

    /**
     * @brief Process inputs during main loop
     * Call once per loop iteration
     */
    void processInputs();

    // ========== Accessor Methods ==========
    /**
     * @brief Get the central state manager
     */
    StateManager& getStateManager() { return state_manager_; }

    /**
     * @brief Get the winch controller
     */
    WinchController& getWinchController() { return winch_controller_; }

    /**
     * @brief Get the home sensor
     */
    HomeSensor& getHomeSensor() { return home_sensor_; }

    /**
     * @brief Get the automatic mode controller
     */
    AutomaticModeController* getAutoModeController() { return auto_mode_controller_; }

    /**
     * @brief Get the remote control
     */
    RemoteControl* getRemoteControl() { return remote_control_; }

    /**
     * @brief Get the emergency stop service
     */
    EmergencyStopService* getEmergencyStopService() { return emergency_stop_service_; }

    /**
     * @brief Get the pulse counter service
     */
    PulseCounterService* getPulseCounterService() { return pulse_counter_service_; }

    /**
     * @brief Get the SignalK service
     */
    SignalKService* getSignalKService() { return signalk_service_; }

private:
    // ========== State Management ==========
    StateManager state_manager_;

    // ========== Hardware Abstraction Layer ==========
    ESP32Motor motor_;
    ESP32Sensor<PinConfig::ANCHOR_HOME> home_sensor_impl_;

    // ========== Business Logic Controllers ==========
    WinchController winch_controller_;
    HomeSensor home_sensor_;
    AutomaticModeController* auto_mode_controller_ = nullptr;
    RemoteControl* remote_control_ = nullptr;

    // ========== Services ==========
    EmergencyStopService* emergency_stop_service_ = nullptr;
    PulseCounterService* pulse_counter_service_ = nullptr;
    SignalKService* signalk_service_ = nullptr;

    // ========== Helper Methods ==========
    void initializeHardware();
    void initializeControllers();
    void initializeServices();
    void attachPulseISR();
    void onEmergencyStopChanged(bool is_active, const char* reason);
};
