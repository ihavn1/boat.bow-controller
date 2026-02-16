// Mock Arduino.h for native testing
// This file provides minimal Arduino definitions for running tests on the host

#ifndef ARDUINO_H
#define ARDUINO_H

#include <stdint.h>
#include <stdlib.h>

// Arduino constants
#ifndef HIGH
#define HIGH 0x1
#endif

#ifndef LOW
#define LOW 0x0
#endif

#ifndef INPUT
#define INPUT 0x0
#endif

#ifndef OUTPUT
#define OUTPUT 0x1
#endif

#ifndef INPUT_PULLUP
#define INPUT_PULLUP 0x2
#endif

// Mock Arduino functions - these are implemented in the test file itself
int digitalRead(uint8_t pin);
void digitalWrite(uint8_t pin, uint8_t val);
void pinMode(uint8_t pin, uint8_t mode);

// Mock delay function (no-op for native tests)
inline void delay(unsigned long ms) {
    // No-op for native testing
}

#endif // ARDUINO_H
