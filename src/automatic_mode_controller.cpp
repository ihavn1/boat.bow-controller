#include "automatic_mode_controller.h"
#include "sensesp/system/local_debug.h"

using namespace sensesp;

AutomaticModeController::AutomaticModeController(WinchController& winch, HomeSensor& home_sensor)
    : winch_(winch),
      home_sensor_(home_sensor),
      enabled_(false),
      target_length_(-1.0f),
      tolerance_(0.2f),
      target_reached_(false) {}

void AutomaticModeController::setEnabled(bool enabled) {
    enabled_ = enabled;
    if (!enabled) {
        winch_.stop();
    }
}

bool AutomaticModeController::isEnabled() const {
    return enabled_;
}

void AutomaticModeController::setTargetLength(float meters) {
    target_length_ = meters;
}

float AutomaticModeController::getTargetLength() const {
    return target_length_;
}

void AutomaticModeController::setTolerance(float meters) {
    tolerance_ = meters;
}

bool AutomaticModeController::consumeTargetReached() {
    bool reached = target_reached_;
    target_reached_ = false;
    return reached;
}

void AutomaticModeController::update(float current_length) {
    if (!enabled_ || target_length_ < 0) {
        return;
    }

    float error = current_length - target_length_;

    // Special case: auto-home (target = 0.0) stops on home sensor, not distance
    if (target_length_ == 0.0f) {
        // Check home sensor directly - don't rely on pulse count
        if (home_sensor_.isHome()) {
            // At home - stop and don't try to move
            return;
        }
        // Only control direction - home sensor will stop the winch
        if (!winch_.isMovingUp()) {
            winch_.moveUp();
        }
        return;
    }

    // Normal distance-based control for non-zero targets
    if (fabs(error) <= tolerance_) {
        // Target reached
        if (winch_.isActive()) {
            winch_.stop();
        }
        enabled_ = false;
        target_reached_ = true;
        debugD("Target %.2f m reached - automatic mode disabled", current_length);
    } else if (error < 0) {
        // Too short - need to deploy more
        if (!winch_.isMovingDown()) {
            winch_.moveDown();
        }
    } else {
        // Too long - need to retrieve
        if (!winch_.isMovingUp()) {
            winch_.moveUp();
        }
    }
}
