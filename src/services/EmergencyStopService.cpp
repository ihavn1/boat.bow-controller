#include "services/EmergencyStopService.h"
#include "sensesp/system/local_debug.h"

using namespace sensesp;

void EmergencyStopService::setActive(bool active, const char* reason) {
    bool was_active = isActive();
    
    // No state change needed
    if (active == was_active) {
        return;
    }
    
    // Update state
    state_manager_.setEmergencyStopActive(active);
    
    if (active) {
        // Emergency stop activated
        winch_controller_.stop();
        if (bow_propeller_controller_) {
            bow_propeller_controller_->stop();
        }
        state_manager_.setAutoModeEnabled(false);
        state_manager_.setManualControl(0);  // Stop manual control
        
        ESP_LOGI("emergency_stop", "ACTIVATED (%s)", reason);
    } else {
        // Emergency stop deactivated
        ESP_LOGI("emergency_stop", "CLEARED (%s)", reason);
    }
    
    // Notify callback if registered
    if (state_change_callback_) {
        state_change_callback_(active, reason);
    }
}
