#include "winch_controller.h"
#include "sensesp/system/local_debug.h"

using namespace sensesp;

void AnchorWinchController::moveUp() {
    if (home_sensor_.isActive()) {
        debugD("Anchor already home - cannot retrieve further");
        stop();
        return;
    }
    motor_.moveUp();
}

void AnchorWinchController::moveDown() {
    motor_.moveDown();
}

void AnchorWinchController::stop() {
    motor_.stop();
}

bool AnchorWinchController::isActive() const {
    return motor_.isActive();
}

bool AnchorWinchController::isMovingUp() const {
    return motor_.isMovingUp();
}

bool AnchorWinchController::isMovingDown() const {
    return motor_.isMovingDown();
}
