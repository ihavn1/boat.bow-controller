#include "hardware/ESP32BowPropellerMotor.h"
#include "sensesp/system/local_debug.h"

using namespace sensesp;

void BowPropellerMotor::initialize() {
    pinMode(PinConfig::BOW_PORT, OUTPUT);
    pinMode(PinConfig::BOW_STARBOARD, OUTPUT);
    stop();
}

void BowPropellerMotor::turnPort() {
    // Ensure starboard is off before turning port (prevent simultaneous activation)
    digitalWrite(PinConfig::BOW_STARBOARD, HIGH);
    digitalWrite(PinConfig::BOW_PORT, LOW);
    active_ = true;
    current_direction_ = Direction::PORT;
    debugD("Bow propeller turning PORT");
}

void BowPropellerMotor::turnStarboard() {
    // Ensure port is off before turning starboard (prevent simultaneous activation)
    digitalWrite(PinConfig::BOW_PORT, HIGH);
    digitalWrite(PinConfig::BOW_STARBOARD, LOW);
    active_ = true;
    current_direction_ = Direction::STARBOARD;
    debugD("Bow propeller turning STARBOARD");
}

void BowPropellerMotor::stop() {
    digitalWrite(PinConfig::BOW_PORT, HIGH);
    digitalWrite(PinConfig::BOW_STARBOARD, HIGH);
    active_ = false;
    current_direction_ = Direction::STOPPED;
    logStopThrottled();
}

bool BowPropellerMotor::isActive() const {
    return active_;
}

BowPropellerMotor::Direction BowPropellerMotor::getCurrentDirection() const {
    return current_direction_;
}

bool BowPropellerMotor::isTurningPort() const {
    return active_ && digitalRead(PinConfig::BOW_PORT) == LOW;
}

bool BowPropellerMotor::isTurningStarboard() const {
    return active_ && digitalRead(PinConfig::BOW_STARBOARD) == LOW;
}

void BowPropellerMotor::logStopThrottled() {
    const unsigned long now_ms = millis();
    if (now_ms - last_stop_log_ms_ >= 5000UL) {
        debugD("Bow propeller stopped");
        last_stop_log_ms_ = now_ms;
    }
}
