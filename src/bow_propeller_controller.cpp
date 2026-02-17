#include "bow_propeller_controller.h"
#include "hardware/ESP32BowPropellerMotor.h"

void BowPropellerController::turnPort() {
    motor_.turnPort();
}

void BowPropellerController::turnStarboard() {
    motor_.turnStarboard();
}

void BowPropellerController::stop() {
    motor_.stop();
}

bool BowPropellerController::isActive() const {
    return motor_.isActive();
}

bool BowPropellerController::isTurningPort() const {
    return motor_.isTurningPort();
}

bool BowPropellerController::isTurningStarboard() const {
    return motor_.isTurningStarboard();
}
