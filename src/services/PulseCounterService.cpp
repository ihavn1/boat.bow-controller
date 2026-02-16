#include "services/PulseCounterService.h"
#include "sensesp_app.h"
#include "sensesp/system/local_debug.h"

using namespace sensesp;

void PulseCounterService::initialize() {
    // Set up repeating task to update pulse count and rode length
    event_loop()->onRepeat(read_delay_ms_, [this]() { this->update(); });
}

void PulseCounterService::update() {
    // Handle home sensor logic
    if (home_sensor_.isHome()) {
        // Anchor is at home
        if (winch_controller_.isMovingUp()) {
            winch_controller_.stop();
            debugD("Anchor home reached - stopped");
        }
        
        if (home_sensor_.justArrived()) {
            // Just arrived at home - reset counter
            state_manager_.setPulseCount(0);
            debugD("Anchor at home - counter reset");
        }
        
        // If auto-mode was targeting home (0.0), disable it
        if (state_manager_.isAutoModeEnabled() &&
            state_manager_.getAutoModeTarget() == 0.0f) {
            state_manager_.setAutoModeEnabled(false);
            debugD("Auto-home reached - automatic mode disabled");
        }
    } else {
        // Keep edge tracking accurate when not at home
        home_sensor_.justLeft();
    }

    // Calculate and update rode length
    long pulse_count = state_manager_.getPulseCount();
    float meters_per_pulse = state_manager_.getMetersPerPulse();
    float meters = pulse_count * meters_per_pulse;
    state_manager_.setRodeLength(meters);

    // Periodic debug output (throttled)
    const unsigned long now_ms = millis();
    if (now_ms - last_debug_ms_ > 5000) {
        debugD("Pulses: %ld, Chain: %.2f m", pulse_count, meters);
        last_debug_ms_ = now_ms;
    }
}
