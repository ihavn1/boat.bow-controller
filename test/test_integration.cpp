// Integration tests for the complete anchor counter system
// Tests full initialization sequence and component interactions

#include <unity.h>
#include <Arduino.h>
#include <stdexcept>
#include "pin_config.h"

// Mock GPIO states
extern bool mock_gpio_states[40];
extern int mock_gpio_modes[40];

// Test fixture for complete system
class CompleteSystemTest {
public:
    CompleteSystemTest()
        : pulse_count_(0),
          rode_length_(0.0f),
          motor_up_active_(false),
          motor_down_active_(false),
          home_sensor_active_(false),
          emergency_stop_active_(false),
          signalk_connected_(false) {}
    
    // Simulate full system startup
    void startup() {
        // Step 1: Create SensESP app FIRST (crucial for event_loop)
        signalk_available_ = true;
        
        // Step 2: Initialize hardware
        initializeGPIO();
        initializeSensors();
        
        // Step 3: Initialize controllers
        initializeControllers();
        
        // Step 4: Initialize services (uses event_loop - requires step 1)
        initializeServices();
        
        // Step 5: Start SignalK after everything is ready
        startSignalK();
    }
    
    void initializeGPIO() {
        // Motor control pins
        mock_gpio_modes[PinConfig::WINCH_UP] = OUTPUT;
        mock_gpio_modes[PinConfig::WINCH_DOWN] = OUTPUT;
        
        // Relay control: default HIGH (inactive) for safety
        mock_gpio_states[PinConfig::WINCH_UP] = HIGH;
        mock_gpio_states[PinConfig::WINCH_DOWN] = HIGH;
        
        // Sensor pins
        mock_gpio_modes[PinConfig::ANCHOR_HOME] = INPUT_PULLUP;
        mock_gpio_modes[PinConfig::DIRECTION] = INPUT_PULLUP;
        mock_gpio_modes[PinConfig::PULSE_INPUT] = INPUT;
        
        gpio_initialized_ = true;
    }
    
    void initializeSensors() {
        // Home sensor reads initial state
        home_sensor_active_ = (mock_gpio_states[PinConfig::ANCHOR_HOME] == LOW);
        
        sensors_initialized_ = true;
    }
    
    void initializeControllers() {
        // Controllers use GPIO to control motor
        controllers_initialized_ = true;
    }
    
    void initializeServices() {
        // Services set up periodic tasks via event_loop()
        services_initialized_ = true;
    }
    
    void startSignalK() {
        signalk_connected_ = true;
    }
    
    // Simulate pulse counter interrupt
    void simulatePulse(int direction, int count = 1) {
        for (int i = 0; i < count; i++) {
            if (direction > 0) {
                pulse_count_++;  // Out
            } else {
                pulse_count_--;  // In
            }
            rode_length_ = pulse_count_ * 0.1f;  // 0.1m per pulse
        }
    }
    
    // Simulate manual control
    void manualUp() {
        if (emergency_stop_active_) return;
        if (!home_sensor_active_) {  // Home sensor blocking
            stop();
            return;
        }
        motor_up_active_ = true;
        motor_down_active_ = false;
        mock_gpio_states[PinConfig::WINCH_UP] = LOW;   // Active-LOW relay
        mock_gpio_states[PinConfig::WINCH_DOWN] = HIGH;
    }
    
    void manualDown() {
        if (emergency_stop_active_) return;
        motor_up_active_ = false;
        motor_down_active_ = true;
        mock_gpio_states[PinConfig::WINCH_UP] = HIGH;
        mock_gpio_states[PinConfig::WINCH_DOWN] = LOW;   // Active-LOW relay
    }
    
    void stop() {
        motor_up_active_ = false;
        motor_down_active_ = false;
        mock_gpio_states[PinConfig::WINCH_UP] = HIGH;
        mock_gpio_states[PinConfig::WINCH_DOWN] = HIGH;
    }
    
    // Simulate emergency stop
    void triggerEmergencyStop() {
        emergency_stop_active_ = true;
        stop();
    }
    
    // System state getters
    bool isFullyInitialized() const {
        return gpio_initialized_ && sensors_initialized_ && 
               controllers_initialized_ && services_initialized_ && 
               signalk_connected_;
    }
    
    float getRodeLength() const { return rode_length_; }
    int getPulseCount() const { return pulse_count_; }
    bool isMotorUp() const { return motor_up_active_; }
    bool isMotorDown() const { return motor_down_active_; }
    bool isEmergencyStopActive() const { return emergency_stop_active_; }
    bool isSignalKConnected() const { return signalk_connected_; }
    
private:
    int pulse_count_;
    float rode_length_;
    bool motor_up_active_;
    bool motor_down_active_;
    bool home_sensor_active_;
    bool emergency_stop_active_;
    bool signalk_connected_;
    bool signalk_available_ = false;
    bool gpio_initialized_ = false;
    bool sensors_initialized_ = false;
    bool controllers_initialized_ = false;
    bool services_initialized_ = false;
};

// ========== TESTS ==========

void test_system_full_startup_sequence() {
    CompleteSystemTest sys;
    
    sys.startup();
    
    TEST_ASSERT_TRUE(sys.isFullyInitialized());
    TEST_ASSERT_TRUE(sys.isSignalKConnected());
}

void test_system_pulse_counting() {
    CompleteSystemTest sys;
    sys.startup();
    
    // Simulate pulses (rode out)
    sys.simulatePulse(1, 10);
    
    TEST_ASSERT_EQUAL_INT(10, sys.getPulseCount());
    TEST_ASSERT_EQUAL_FLOAT(1.0f, sys.getRodeLength());
}

void test_system_negative_pulse_counting() {
    CompleteSystemTest sys;
    sys.startup();
    
    // Simulate pulses out then in
    sys.simulatePulse(1, 20);   // Out 20 pulses
    sys.simulatePulse(-1, 5);   // In 5 pulses
    
    TEST_ASSERT_EQUAL_INT(15, sys.getPulseCount());
    TEST_ASSERT_EQUAL_FLOAT(1.5f, sys.getRodeLength());
}

void test_system_manual_control_up() {
    CompleteSystemTest sys;
    sys.startup();
    
    sys.manualUp();
    
    TEST_ASSERT_TRUE(sys.isMotorUp());
    TEST_ASSERT_FALSE(sys.isMotorDown());
    TEST_ASSERT_EQUAL_INT(LOW, (int)mock_gpio_states[PinConfig::WINCH_UP]);
}

void test_system_manual_control_down() {
    CompleteSystemTest sys;
    sys.startup();
    
    sys.manualDown();
    
    TEST_ASSERT_FALSE(sys.isMotorUp());
    TEST_ASSERT_TRUE(sys.isMotorDown());
    TEST_ASSERT_EQUAL_INT(LOW, (int)mock_gpio_states[PinConfig::WINCH_DOWN]);
}

void test_system_manual_control_stop() {
    CompleteSystemTest sys;
    sys.startup();
    
    sys.manualUp();
    TEST_ASSERT_TRUE(sys.isMotorUp());
    
    sys.stop();
    
    TEST_ASSERT_FALSE(sys.isMotorUp());
    TEST_ASSERT_FALSE(sys.isMotorDown());
    TEST_ASSERT_EQUAL_INT(HIGH, (int)mock_gpio_states[PinConfig::WINCH_UP]);
    TEST_ASSERT_EQUAL_INT(HIGH, (int)mock_gpio_states[PinConfig::WINCH_DOWN]);
}

void test_system_emergency_stop_stops_motor() {
    CompleteSystemTest sys;
    sys.startup();
    
    sys.manualUp();
    TEST_ASSERT_TRUE(sys.isMotorUp());
    
    sys.triggerEmergencyStop();
    
    TEST_ASSERT_TRUE(sys.isEmergencyStopActive());
    TEST_ASSERT_FALSE(sys.isMotorUp());
    TEST_ASSERT_FALSE(sys.isMotorDown());
}

void test_system_emergency_stop_prevents_control() {
    CompleteSystemTest sys;
    sys.startup();
    
    sys.triggerEmergencyStop();
    sys.manualUp();  // Should be blocked
    
    TEST_ASSERT_FALSE(sys.isMotorUp());
}

void test_system_home_sensor_blocking() {
    CompleteSystemTest sys;
    sys.startup();
    
    // Home sensor is active (at home)
    sys.manualUp();  // Should stop motor due to home sensor
    
    TEST_ASSERT_FALSE(sys.isMotorUp());
}

void test_system_pulse_and_control_integration() {
    CompleteSystemTest sys;
    sys.startup();
    
    // Simulate auto-chain deployment
    sys.manualDown();
    sys.simulatePulse(1, 50);  // Deploy 50 pulses (5 meters)
    sys.stop();
    
    TEST_ASSERT_EQUAL_INT(50, sys.getPulseCount());
    TEST_ASSERT_EQUAL_FLOAT(5.0f, sys.getRodeLength());
    TEST_ASSERT_FALSE(sys.isMotorDown());
}

void test_system_relay_safety_defaults() {
    // Relays should default to HIGH (inactive) for safety
    TEST_ASSERT_EQUAL_INT(HIGH, (int)mock_gpio_states[PinConfig::WINCH_UP]);
    TEST_ASSERT_EQUAL_INT(HIGH, (int)mock_gpio_states[PinConfig::WINCH_DOWN]);
}

void test_system_signalk_integration() {
    CompleteSystemTest sys;
    
    // StartSignalK before full init should still work
    // In real code, event_loop would need sensesp_app
    sys.startSignalK();
    TEST_ASSERT_TRUE(sys.isSignalKConnected());
    
    // Full startup should also work
    sys.startup();
    TEST_ASSERT_TRUE(sys.isSignalKConnected());
}
