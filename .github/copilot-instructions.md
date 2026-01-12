# Copilot Instructions

This document provides guidance for AI agents to effectively contribute to the `boat-bowsensors` codebase.

## 1. The Big Picture: Architecture & Core Concepts

This project is an ESP32-based anchor chain counter and windlass controller. It's built on the **SensESP framework**, which uses a reactive, dataflow-oriented model.

- **Core Logic**: The main application logic resides in `src/main.cpp`. It initializes hardware, sets up sensor processing pipelines, and handles control logic.
- **SensESP Framework**: The application is built using `SensESPAppBuilder`. The architecture revolves around **Sensors**, **Transforms**, and **Consumers**.
  - **Sensors** (e.g., the custom `PulseCounter` class in `main.cpp`) produce data.
  - **Transforms** (e.g., `LambdaTransform`) process data flowing through the system.
  - **Consumers** (e.g., `SKOutputFloat`, `LambdaConsumer`) are the final destination for data, often sending it to the SignalK server or triggering actions.
- **SignalK Integration**: The device communicates with a SignalK server for remote control and monitoring. Key input (command) and output (status) paths are defined in `src/main.cpp` and documented in `README.md`. All interactions with the outside world happen via these SignalK paths.
- **Control Logic**: The system has two primary modes:
  1.  **Manual Mode**: Direct control of the winch via `navigation.anchor.manualControl`.
  2.  **Automatic Mode**: An "arm-and-fire" system. A target length is first "armed" (`navigation.anchor.targetRodeCommand`), and then automatic control is "fired" (`navigation.anchor.automaticModeCommand`). The logic inside the `PulseCounter::update()` method then manages the winch to reach the target.

## 2. Developer Workflows

The project uses **PlatformIO** for all build and development tasks.

- **Dependencies**: All dependencies are managed via `platformio.ini`. The primary dependency is `SignalK/SensESP`.
- **Building**: To build the project, run:
  ```bash
  pio run -e az-delivery-devkit-v4
  ```
- **Uploading**: To build and upload the firmware to the device:
  ```bash
  pio run -e az-delivery-devkit-v4 --target upload
  ```
- **Monitoring**: To view serial output from the device:
  ```bash
  pio device monitor
  ```
- **Testing**: The project uses the **Unity** test framework. Tests are located in the `test/` directory. They rely on mocking Arduino framework functions (`digitalRead`, `digitalWrite`) to simulate hardware states.
  - To run tests, use the PlatformIO test runner:
    ```bash
    pio test
    ```

## 3. Code Conventions & Patterns

- **Hardware Abstraction**: All GPIO pin definitions are at the top of `src/main.cpp`. All hardware interaction (reading sensors, controlling relays) is centralized in this file.
- **State Management**: Global variables in `main.cpp` (e.g., `pulse_count`, `automatic_mode_enabled`) hold the primary state. The `PulseCounter` class contains the core logic for reading sensors and implementing the automatic control loop.
- **Configuration**: User-facing configuration (like calibration values) is exposed through the SensESP web UI. This is done by creating `ConfigItem` objects in `setup()`, for example, the `meters_per_pulse` setting.
- **SignalK Communication**:
  - To send data to SignalK, connect a producer to an `SKOutput` class (e.g., `SKOutputFloat`).
  - To receive data from SignalK, create an `SKListener` (e.g., `IntSKListener`) and connect it to a `LambdaConsumer` or `LambdaTransform` to process the incoming value.
- **Asynchronous Operations**: The main `loop()` is minimal. All recurring tasks, like reading the pulse counter, are handled by the SensESP event loop, which is set up via `event_loop()->onRepeat()` in the `PulseCounter` constructor. Interrupts (`pulseISR`) handle high-frequency hardware events.
````