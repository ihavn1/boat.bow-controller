#include "services/BoatAnchorApp.h"
#include "services/SignalKService.h"

// Global app instance (needed for ISR access)
static BoatAnchorApp* g_app = nullptr;

// Interrupt Service Routine - Pulse Counter with Direction Sensing
// Called from ISR context - must be very fast
void IRAM_ATTR pulseISR() {
    if (!g_app) return;
    
    delayMicroseconds(10);  // Allow direction signal to stabilize
    
    if (digitalRead(PinConfig::DIRECTION)) {
        g_app->getStateManager().incrementPulse();  // Chain out
    } else {
        g_app->getStateManager().decrementPulse();  // Chain in
    }
}

BoatAnchorApp::BoatAnchorApp()
    : motor_(),
      home_sensor_impl_(),
      winch_controller_(motor_, home_sensor_impl_),
      home_sensor_(home_sensor_impl_) {
    g_app = this;  // Store global pointer for ISR
}

void BoatAnchorApp::initialize() {
    initializeHardware();
    initializeControllers();
    initializeServices();
    attachPulseISR();

    debugD("=== Boat Anchor App Initialized ===");
    debugD("Pulse input: GPIO %d, Direction: GPIO %d", 
           PinConfig::PULSE_INPUT, PinConfig::DIRECTION);
    debugD("Winch: UP=GPIO %d, DOWN=GPIO %d", 
           PinConfig::WINCH_UP, PinConfig::WINCH_DOWN);
    debugD("Home sensor: GPIO %d", PinConfig::ANCHOR_HOME);
    debugD("Remote outputs: OUT1=GPIO %d, OUT2=GPIO %d", 
           PinConfig::REMOTE_OUT1, PinConfig::REMOTE_OUT2);
}

void BoatAnchorApp::startSignalK() {
    if (!signalk_service_) {
        debugD("ERROR: SignalK service not initialized!");
        return;
    }

    signalk_service_->startConnectionMonitoring();
    debugD("SignalK integration started - waiting for connection...");
}

void BoatAnchorApp::processInputs() {
    // Process physical remote control inputs
    if (remote_control_) {
        remote_control_->processInputs();
    }
    
    // Tick the SensESP event loop
    event_loop()->tick();
}

void BoatAnchorApp::initializeHardware() {
    // SAFETY FIRST: Initialize all hardware to safe inactive state
    
    // Initialize motor first
    motor_.initialize();
    home_sensor_impl_.initialize();
    
    // Initialize remote spare outputs (active-LOW logic)
    pinMode(PinConfig::REMOTE_OUT1, OUTPUT);
    pinMode(PinConfig::REMOTE_OUT2, OUTPUT);
    digitalWrite(PinConfig::REMOTE_OUT1, HIGH);
    digitalWrite(PinConfig::REMOTE_OUT2, HIGH);

    debugD("Hardware initialized - all outputs inactive");
}

void BoatAnchorApp::initializeControllers() {
    // Initialize automatic mode controller
    auto_mode_controller_ = new AutomaticModeController(winch_controller_, home_sensor_);
    state_manager_.setMetersPerPulse(0.01f);
    auto_mode_controller_->setTolerance(state_manager_.getMetersPerPulse() * 2.0);
    
    // Initialize remote control
    remote_control_ = new RemoteControl(state_manager_, winch_controller_);
    remote_control_->initialize();

    debugD("Controllers initialized");
}

void BoatAnchorApp::initializeServices() {
    // Initialize emergency stop service (without callback - SignalK will handle updates)
    emergency_stop_service_ = new EmergencyStopService(state_manager_, winch_controller_);
    // Note: onStateChange callback requires a raw function pointer, not a lambda
    // Instead, SignalK integration will poll the state and emit updates
    
    // Initialize pulse counter service
    pulse_counter_service_ = new PulseCounterService(state_manager_, winch_controller_, 
                                                      home_sensor_, 100);
    pulse_counter_service_->initialize();
    
    // Initialize SignalK service
    signalk_service_ = new SignalKService(state_manager_, winch_controller_, home_sensor_,
                                         auto_mode_controller_, emergency_stop_service_,
                                         pulse_counter_service_);
    signalk_service_->initialize();

    debugD("Services initialized");
}

void BoatAnchorApp::attachPulseISR() {
    // Configure the pulse input pin and direction pin
    pinMode(PinConfig::PULSE_INPUT, INPUT_PULLUP);
    pinMode(PinConfig::DIRECTION, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PinConfig::PULSE_INPUT), pulseISR, RISING);

    debugD("Pulse ISR attached to GPIO %d", PinConfig::PULSE_INPUT);
}

void BoatAnchorApp::onEmergencyStopChanged(bool is_active, const char* reason) {
    // Update SignalK status when emergency stop state changes
    if (signalk_service_) {
        auto status_value = signalk_service_->getEmergencyStopStatus();
        if (status_value) {
            status_value->set(is_active);
            status_value->notify();  // Force emission even if unchanged
        }
    }
    
    if (is_active) {
        debugD("EMERGENCY STOP ACTIVATED (%s)", reason);
    } else {
        debugD("Emergency stop deactivated");
    }
}
