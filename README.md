# Boat Bow Sensors - Anchor Chain Counter & Windlass Control

An ESP32-based anchor chain counter and automatic windlass control system with SignalK integration.

## Features

### Anchor Chain Length Monitoring
- **Bidirectional pulse counting** - Accurate chain length measurement with direction sensing
- **Real-time tracking** - Continuous monitoring of deployed chain length
- **SignalK integration** - Reports current rode length to `navigation.anchor.currentRode`
- **Web-based calibration** - Configurable meters-per-pulse conversion factor

### Automatic Windlass Control
- **Arm and fire operation** - Set target first, then enable automatic mode when ready
- **Intelligent control** - Automatically deploys or retrieves chain to reach target
- **Auto-disable on completion** - Returns to manual mode when target reached
- **Precision stopping** - Stops within ±2 pulses (±0.2m default) of target

### Safety Features
- **Home position detection** - Prevents over-retrieval with dedicated sensor
- **Automatic counter reset** - Resets to zero when anchor reaches home
- **Manual override protection** - Manual controls disabled during automatic operation
- **Status feedback** - Real-time mode and target status via SignalK

## Hardware Requirements

- **ESP32 Board**: MH ET LIVE ESP32MiniKit (or compatible)
- **Pulse Sensor**: Chain counter pulse sensor (e.g., Hall effect or optical)
- **Direction Sensor**: Detects chain in/out direction
- **Home Position Sensor**: Detects when anchor is fully retrieved
- **Relay/Contactor**: For windlass motor control (2 channels: UP/DOWN)

## GPIO Pin Configuration

| Function | GPIO | Direction | Description |
|----------|------|-----------|-------------|
| Pulse Input | 25 | Input | Chain counter pulse sensor |
| Direction | 26 | Input | HIGH = chain out, LOW = chain in |
| Anchor Home | 33 | Input | LOW = anchor at home position |
| Winch UP | 27 | Output | Activate to retrieve chain (active LOW) |
| Winch DOWN | 14 | Output | Activate to deploy chain (active LOW) |
| Remote UP | 12 | Input | Manual UP from remote control (active HIGH) |
| Remote DOWN | 13 | Input | Manual DOWN from remote control (active HIGH) |
| Remote Func 3| 15 | Input | Spare remote input (reserved, active HIGH) |
| Remote Func 4| 16 | Input | Spare remote input (reserved, active HIGH) |
| Remote Out 1 | 4 | Output | Spare remote output (reserved, active LOW) |
| Remote Out 2 | 5 | Output | Spare remote output (reserved, active LOW) |

## Quick Start

1. **Hardware Setup**: Connect sensors and relays according to GPIO pin configuration
2. **Flash Firmware**: Upload code to ESP32 using PlatformIO
3. **Configure WiFi**: Connect to `bow-controller` access point on first boot
4. **Calibrate**: Set meters-per-pulse value via web interface at `http://bow-controller.local/`
5. **SignalK Integration**: Device automatically connects and publishes/subscribes to SignalK paths

## SignalK Paths

### Outputs (Device → SignalK)
| Path | Type | Units | Description |
|------|------|-------|-------------|
| `navigation.anchor.currentRode` | float | m | Current chain length (meters) |
| `navigation.anchor.automaticModeStatus` | float | - | Automatic mode state (1.0=enabled, 0.0=disabled) |
| `navigation.anchor.targetRodeStatus` | float | m | Current target length (meters) |
| `navigation.anchor.manualControlStatus` | int | - | Manual control echo (1=UP, 0=STOP, -1=DOWN) |

### Inputs (SignalK → Device)
| Path | Type | Values | Description |
|------|------|--------|-------------|
| `navigation.anchor.automaticModeCommand` | float | >0.5=enable, ≤0.5=disable | Enable/disable automatic control |
| `navigation.anchor.targetRodeCommand` | float | meters | Arm target length for automatic mode |
| `navigation.anchor.manualControl` | int | 1=UP, 0=STOP, -1=DOWN | Manual windlass control |
| `navigation.anchor.resetRode` | bool | true | Reset counter to zero |

## Usage Examples

### Automatic Deployment (Arm and Fire)
```json
// 1. Arm target (prepare but don't start)
{"path": "navigation.anchor.targetRodeCommand", "value": 15.0}

// 2. Fire when ready (starts windlass automatically)
{"path": "navigation.anchor.automaticModeCommand", "value": 1.0}

// System will automatically disable when target reached
```

### Manual Windlass Control
```json
// Retrieve chain
{"path": "navigation.anchor.manualControl", "value": 1}
// ... existing content ...
{"path": "navigation.anchor.manualControl", "value": -1}
```

### Physical Remote Control
The system supports a physical remote control for manual operation. The winch will be active as long as the corresponding remote button is held down.
- The remote control overrides SignalK manual commands.
- The remote control will NOT operate if the system is in automatic mode.
- The `Anchor Home` safety feature remains active, preventing retrieval if the anchor is already home.

### Emergency Stop
```json
// Immediately disable automatic mode and stop winch
{"path": "navigation.anchor.automaticModeCommand", "value": 0}
```

## Documentation

See [ANCHOR_CHAIN_USAGE.md](ANCHOR_CHAIN_USAGE.md) for detailed usage instructions, REST API examples, safety notes, and troubleshooting.

## Technology Stack

- **Platform**: ESP32 (Arduino framework)
- **Framework**: [SensESP v3.1.1](https://github.com/SignalK/SensESP)
- **Protocol**: SignalK WebSocket/HTTP
- **Build System**: PlatformIO
- **Web Interface**: Built-in configuration UI

## Development

```bash
# Build project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor

# Run tests (requires ESP32 connected via USB)
pio test -e test
```

### Testing

The project uses the **Unity** test framework with 30+ comprehensive tests covering:
- Pulse counting and ISR behavior
- Physical remote control operations
- Home sensor safety features
- Manual and automatic winch control
- Mode transitions and edge cases

**Note:** Tests mock GPIO hardware but still require an ESP32 board connected via USB to execute. The tests run on the hardware but don't require actual sensors or winches connected.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

ihavn1" 
