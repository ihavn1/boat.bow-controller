#include "home_sensor.h"

bool HomeSensor::isHome() const {
    return sensor_.isActive();
}

bool HomeSensor::justArrived() {
    return sensor_.justActivated();
}

bool HomeSensor::justLeft() {
    return sensor_.justDeactivated();
}
