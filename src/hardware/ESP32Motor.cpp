#include "hardware/ESP32Motor.h"
#include "sensesp/system/local_debug.h"

using namespace sensesp;

void ESP32Motor::initialize() {
    pinMode(PinConfig::WINCH_UP, OUTPUT);
    pinMode(PinConfig::WINCH_DOWN, OUTPUT);
    stop();
}

void ESP32Motor::moveUp() {
    digitalWrite(PinConfig::WINCH_DOWN, HIGH);
    digitalWrite(PinConfig::WINCH_UP, LOW);
    active_ = true;
    current_direction_ = Direction::UP;
    debugD("Motor UP activated");
}

void ESP32Motor::moveDown() {
    digitalWrite(PinConfig::WINCH_UP, HIGH);
    digitalWrite(PinConfig::WINCH_DOWN, LOW);
    active_ = true;
    current_direction_ = Direction::DOWN;
    debugD("Motor DOWN activated");
}

void ESP32Motor::stop() {
    digitalWrite(PinConfig::WINCH_UP, HIGH);
    digitalWrite(PinConfig::WINCH_DOWN, HIGH);
    active_ = false;
    current_direction_ = Direction::STOPPED;
    logStopThrottled();
}

bool ESP32Motor::isActive() const {
    return active_;
}

IMotor::Direction ESP32Motor::getCurrentDirection() const {
    return current_direction_;
}

bool ESP32Motor::isMovingUp() const {
    return active_ && digitalRead(PinConfig::WINCH_UP) == LOW;
}

bool ESP32Motor::isMovingDown() const {
    return active_ && digitalRead(PinConfig::WINCH_DOWN) == LOW;
}

void ESP32Motor::logStopThrottled() {
    const unsigned long now_ms = millis();
    if (now_ms - last_stop_log_ms_ >= 5000UL) {
        debugD("Motor stopped");
        last_stop_log_ms_ = now_ms;
    }
}
