# Copilot Instructions

This document provides guidance for AI agents to effectively contribute to the `boat.bow-controller` codebase.

## 1. The Big Picture: Architecture & Core Concepts

This project is an ESP32-based anchor chain counter and windlass controller. It's built on the **SensESP framework**, which uses a reactive, dataflow-oriented model.

- **Core Logic**: The main application logic resides in `src/main.cpp`. It initializes hardware controllers and sets up SignalK integration pipelines.
- **Modular Architecture**: The code follows SOLID principles with dedicated classes in `include/`:
  - **WinchController** - Encapsulates winch motor control with built-in safety checks (home sensor blocking)
  - **HomeSensor** - Monitors anchor home position and detects state changes
  - **AutomaticModeController** - Manages automatic positioning logic to reach target rode length
  - **RemoteControl** - Processes physical remote control inputs
  - **PinConfig** - Centralizes all GPIO pin definitions as constants
  - **PulseCounter** (in main.cpp) - Reads pulse counts, converts to meters, coordinates all controllers
- **SensESP Framework**: The application is built using `SensESPAppBuilder`. The architecture revolves around **Sensors**, **Transforms**, and **Consumers**.
  - **Sensors** (e.g., the custom `PulseCounter` class) produce data.
  - **Transforms** (e.g., `LambdaTransform`) process data flowing through the system.
  - **Consumers** (e.g., `SKOutputFloat`, `LambdaConsumer`) are the final destination for data, often sending it to the SignalK server or triggering actions.
- **SignalK Integration**: The device communicates with a SignalK server for remote control and monitoring. Key input (command) and output (status) paths are defined in `src/main.cpp` and documented in `README.md`. All interactions with the outside world happen via these SignalK paths.
- **Control Logic**: The system has three control modes that coexist:
  1.  **Manual Mode** (SignalK): Remote control via `navigation.anchor.manualControl` (1=UP, 0=STOP, -1=DOWN). Handled by lambdas calling `winch_controller` methods.
  2.  **Physical Remote**: Direct hardware buttons processed by `RemoteControl::processInputs()` every loop iteration. These STOP the winch when released, overriding SignalK commands.
  3.  **Automatic Mode**: An "arm-and-fire" system managed by `AutomaticModeController`. A target length is first "armed" (`navigation.anchor.targetRodeCommand`), and then automatic control is "fired" (`navigation.anchor.automaticModeCommand`). The `AutomaticModeController::update()` method manages the winch to reach the target. Automatic mode blocks both manual modes.

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
  - **Testing Pattern**: Tests in `test/test_anchor_counter.cpp` mock the entire Arduino hardware layer. Mock arrays (`mock_gpio_states[]`, `mock_gpio_modes[]`) simulate pin states. Test functions replicate core logic from classes to verify behavior without hardware.
  - **Test Structure**: Each test follows a clear naming pattern (`test_<feature>_<scenario>`) and uses Unity assertions (`TEST_ASSERT_TRUE`, `TEST_ASSERT_EQUAL_INT32`, etc.). Tests cover: pulse counting (increment/decrement), home sensor logic, manual/automatic mode control, arm-and-fire sequence, and target reaching.

## 3. Code Conventions & Patterns

- **Modular Design**: Hardware control is separated into focused classes in `include/` directory:
  - `pin_config.h` - All GPIO pin definitions as `constexpr` constants in `PinConfig` struct (includes reserved spare I/O pins)
  - `winch_controller.h` - Winch motor control with safety checks
  - `home_sensor.h` - Home position detection and state tracking
  - `automatic_mode_controller.h` - Automatic positioning controller
  - `remote_control.h` - Physical remote input processing and spare output initialization
- **Relay Control Pattern**: Winch and spare outputs use **active-LOW logic** - write `LOW` to activate, `HIGH` to deactivate. This is critical for safety: pins default to HIGH (inactive) on boot. Implemented in `WinchController` class and spare outputs (GPIO 4, 5).
- **State Management**: Minimal global state - only `pulse_count` (ISR access), `config_meters_per_pulse`, and SignalK pointer. Controller state is encapsulated in class instances (`winch_controller`, `home_sensor`, `auto_mode_controller`, `remote_control`).
- **Interrupt-Driven Pulse Counting**: The `pulseISR()` function calls `remote_control->processInputs()` for physical remote polling and then `event_loop()->tick()`. All recurring tasks, like reading the pulse counter, are handled by the SensESP event loop, which is set up via `event_loop()->onRepeat()` in the `PulseCounter` constructor. Interrupts (`pulseISR`) handle high-frequency hardware events.
- **Class Initialization Pattern**: Controllers are initialized in `setup()`:
  ```cpp
  winch_controller.initialize();          // Set up GPIO pins
  home_sensor.initialize();                // Configure home sensor
  auto_mode_controller = new AutomaticModeController(winch_controller);
  remote_control = new RemoteControl(winch_controller);
  ```
- **Dependency Injection**: Classes receive dependencies through constructors (e.g., `AutomaticModeController` receives `WinchController&`), enabling loose coupling and testability.
- **SignalK Communication**:
  - To send data to SignalK, connect a producer to an `SKOutput` class (e.g., `SKOutputFloat`).
  - To receive data from SignalK, create an `SKListener` (e.g., `IntSKListener`) and connect it to a `LambdaConsumer` or `LambdaTransform` to process the incoming value.
- **Asynchronous Operations**: The main `loop()` is minimal - it only calls `handleManualInputs()` for physical remote polling and then `event_loop()->tick()`. All recurring tasks, like reading the pulse counter, are handled by the SensESP event loop, which is set up via `event_loop()->onRepeat()` in the `PulseCounter` constructor. Interrupts (`pulseISR`) handle high-frequency hardware events.
````