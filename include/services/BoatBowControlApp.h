#pragma once

#include "StateManager.h"
#include "PulseCounterService.h"
#include "EmergencyStopService.h"
#include "hardware/ESP32Motor.h"
#include "hardware/ESP32Sensor.h"
#include "hardware/ESP32BowPropellerMotor.h"
#include "winch_controller.h"
#include "bow_propeller_controller.h"
#include "home_sensor.h"
#include "automatic_mode_controller.h"
#include "remote_control.h"
#include "pin_config.h"

// Forward declaration to avoid including SignalKService.h which pulls in sensesp_app_builder.h
class SignalKService;

/**
 * @file BoatBowControlApp.h
 * @brief Main application orchestrator for anchor and bow control systems
 * 
 * Coordinates initialization and management of all system components:
 * - Hardware abstraction (motors, sensors, relays)
 * - Business logic controllers (winch, home, auto mode, remote, bow propeller)
 * - Service layer (state management, emergency stop, pulse counting, SignalK)
 * 
 * Follows the orchestrator pattern to encapsulate system complexity and provide
 * a simple interface for setup and operation.
 */
class BoatBowControlApp {
public:
    /**
     * @brief Construct the application without initializing
     * Call initialize() afterwards in setup()
     */
    BoatBowControlApp();

    /**
     * @brief Initialize all hardware and services
     * Must be called during Arduino setup()
     * Initializes (in order):
     *   1. Hardware (GPIO, pins)
     *   2. Controllers (AnchorWinchController, HomeSensor, AutomaticModeController, RemoteControl, BowPropeller)
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
     * @brief Get the anchor winch controller
     */
    AnchorWinchController& getWinchController() { return winch_controller_; }

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
     * @brief Get the bow propeller controller
     */
    BowPropellerController* getBowPropellerController() { return bow_propeller_controller_; }

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

    /**
     * @brief Handle emergency stop state changes
     */
    void onEmergencyStopChanged(bool is_active, const char* reason);

private:
    // ========== State Management ==========
    StateManager state_manager_;

    // ========== Hardware Abstraction Layer ==========
    ESP32Motor motor_;
    ESP32Sensor<PinConfig::ANCHOR_HOME> home_sensor_impl_;
    BowPropellerMotor bow_propeller_motor_;

    // ========== Business Logic Controllers ==========
    AnchorWinchController winch_controller_;
    HomeSensor home_sensor_;
    AutomaticModeController* auto_mode_controller_ = nullptr;
    RemoteControl* remote_control_ = nullptr;
    BowPropellerController* bow_propeller_controller_ = nullptr;

    // ========== Services ==========
    EmergencyStopService* emergency_stop_service_ = nullptr;
    PulseCounterService* pulse_counter_service_ = nullptr;
    SignalKService* signalk_service_ = nullptr;

    // ========== Helper Methods ==========
    void initializeHardware();
    void initializeControllers();
    void initializeServices();
    void attachPulseISR();
};
