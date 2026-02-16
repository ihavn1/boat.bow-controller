#include "winch_controller.h"
#include "sensesp/system/local_debug.h"

using namespace sensesp;

void WinchController::moveUp() {
    if (home_sensor_.isActive()) {
        debugD("Anchor already home - cannot retrieve further");
        stop();
        return;
    }
    motor_.moveUp();
}

void WinchController::moveDown() {
    motor_.moveDown();
}

void WinchController::stop() {
    motor_.stop();
}

bool WinchController::isActive() const {
    return motor_.isActive();
}

bool WinchController::isMovingUp() const {
    return motor_.isMovingUp();
}

bool WinchController::isMovingDown() const {
    return motor_.isMovingDown();
}
