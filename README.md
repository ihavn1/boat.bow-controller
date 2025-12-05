"# Boat Bow Sensors - Anchor Chain Counter & Windlass Control

An ESP32-based anchor chain counter and automatic windlass control system with SignalK integration.

## Features

### Anchor Chain Length Monitoring
- **Pulse-based counting** - Accurate chain length measurement using pulse sensor
- **Bidirectional tracking** - Counts up when deploying, down when retrieving
- **SignalK integration** - Reports current rode length to `navigation.anchor.currentRode`
- **Automatic calibration** - Configurable meters-per-pulse via web interface

### Automatic Windlass Control
- **Target-based deployment** - Set desired chain length via SignalK
- **Enable/disable mode** - Separate control for automatic operation
- **Smart direction control** - Automatically deploys or retrieves to reach target
- **Tolerance-based stopping** - Stops within ±2 pulses of target

### Safety Features
- **Home position detection** - Prevents over-retrieval with dedicated sensor
- **Automatic counter reset** - Resets to zero when anchor reaches home position
- **Manual operation support** - Full manual control with automatic mode disabled
- **Hardware limit protection** - Cannot retrieve past home position

## Hardware Requirements

- **ESP32 Board**: MH ET LIVE ESP32MiniKit (or compatible)
- **Pulse Sensor**: Chain counter pulse sensor (e.g., Hall effect or optical)
- **Direction Sensor**: Detects chain in/out direction
- **Home Position Sensor**: Detects when anchor is fully retrieved
- **Relay/Contactor**: For windlass motor control (2 channels: UP/DOWN)

## GPIO Pin Configuration

| Function | GPIO | Description |
|----------|------|-------------|
| Pulse Input | 25 | Chain counter pulse sensor |
| Direction | 26 | HIGH = chain out, LOW = chain in |
| Anchor Home | 33 | LOW = anchor at home position |
| Winch UP | 27 | Activate to retrieve chain |
| Winch DOWN | 14 | Activate to deploy chain |

## Quick Start

1. **Hardware Setup**: Connect sensors and relays according to GPIO pin configuration
2. **Flash Firmware**: Upload code to ESP32 using PlatformIO
3. **Configure WiFi**: Connect to `bow-sensors` access point on first boot
4. **Calibrate**: Set meters-per-pulse value via web interface at `http://bow-sensors.local/`
5. **SignalK Integration**: Device automatically connects and publishes/subscribes to SignalK paths

## SignalK Paths

### Output (Device → SignalK)
- `navigation.anchor.currentRode` - Current chain length in meters

### Inputs (SignalK → Device)
- `navigation.anchor.automaticModeCommand` - Enable/disable automatic control (float: >0.5=enable, <=0.5=disable) *Note: FloatSKListener doesn't work with Node-RED's signalk-send-pathvalue node*
- `navigation.anchor.targetRodeCommand` - Set target chain length in meters (float) *Note: FloatSKListener doesn't work with Node-RED's signalk-send-pathvalue node*
- `navigation.anchor.manualControl` - Manual windlass control (integer: 1=UP, 0=STOP, -1=DOWN)
- `navigation.anchor.resetRode` - Reset counter to zero (boolean)

## Usage Examples

### Deploy 15 meters automatically
```json
// 1. Enable automatic mode (send value > 0.5 to enable, <= 0.5 to disable)
{"path": "navigation.anchor.automaticModeCommand", "value": 1.0}

// 2. Set target
{"path": "navigation.anchor.targetRodeCommand", "value": 15.0}
```

**Note**: FloatSKListener doesn't receive values from Node-RED's signalk-send-pathvalue node. Consider using separate enable/disable paths or preset target buttons instead.

### Manual windlass control
```json
// Retrieve chain (UP)
{"path": "navigation.anchor.manualControl", "value": 1}

// Stop windlass
{"path": "navigation.anchor.manualControl", "value": 0}

// Deploy chain (DOWN)
{"path": "navigation.anchor.manualControl", "value": -1}
```

### Stop and disable automatic mode
```json
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
```

## License

[Add your license here]

## Author

[Add your name/contact here]" 
