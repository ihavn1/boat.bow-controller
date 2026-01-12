# Anchor Chain Counter - Usage Guide

## Overview
ESP32-based anchor chain counter with automatic windlass control via SignalK. Features "arm and fire" operation where you set the target first, then enable automatic mode when ready to deploy or retrieve.

## SignalK Paths

### Output Paths (Device → SignalK)
| Path | Type | Units | Update Rate | Description |
|------|------|-------|-------------|-------------|
| `navigation.anchor.currentRode` | float | m | 1s | Current chain length (meters) |
| `navigation.anchor.automaticModeStatus` | float | - | on change | Auto mode state (1.0=enabled, 0.0=disabled) |
| `navigation.anchor.targetRodeStatus` | float | m | on change | Armed target length (meters) |
| `navigation.anchor.manualControlStatus` | int | - | on change | Manual control echo (1=UP, 0=STOP, -1=DOWN) |

### Input Paths (SignalK → Device)
| Path | Type | Values | Description |
|------|------|--------|-------------|
| `navigation.anchor.automaticModeCommand` | float | >0.5=enable, ≤0.5=disable | Enable/disable automatic control |
| `navigation.anchor.targetRodeCommand` | float | meters | Arm target for automatic mode |
| `navigation.anchor.manualControl` | int | 1=UP, 0=STOP, -1=DOWN | Manual windlass control |
| `navigation.anchor.resetRode` | bool | true | Reset counter to zero |

## Usage Examples

### Monitor Current Chain Length
Subscribe to `navigation.anchor.currentRode` in your SignalK dashboard for real-time chain length monitoring (updates every second).

### Automatic Windlass Control (Arm and Fire)

The recommended workflow is to **arm** the target first, then **fire** when ready:

#### Step 1: Arm Target (Prepare but don't start)
```json
{
  "context": "vessels.self",
  "updates": [{
    "values": [{
      "path": "navigation.anchor.targetRodeCommand",
      "value": 15.0
    }]
  }]
}
```

This sets the target to 15 meters but doesn't start the windlass. The target is "armed" and ready.

#### Step 2: Fire (Enable automatic mode to start)
```json
{
  "context": "vessels.self",
  "updates": [{
    "values": [{
      "path": "navigation.anchor.automaticModeCommand",
      "value": 1.0
    }]
  }]
}
```

The windlass immediately starts moving toward the armed target. When the target is reached (±0.2m), the system automatically:
- Stops the windlass
- Disables automatic mode
- Updates `automaticModeStatus` to 0.0

#### Emergency Stop
```json
{
  "context": "vessels.self",
  "updates": [{
    "values": [{
      "path": "navigation.anchor.automaticModeCommand",
      "value": 0.0
    }]
  }]
}
```

Immediately stops the windlass and disables automatic mode.

#### Alternative Workflow
You can also enable automatic mode first, then set the target - the windlass will start immediately when the target is set.

### Manual Windlass Control

Manual control can be performed via SignalK or a physical remote. Both methods only work when automatic mode is **disabled**.

#### SignalK Manual Control

SignalK manual control uses a single path with integer values.

| Value | Action |
|-------|--------|
| `1` | Retrieve chain (UP) |
| `0` | Stop windlass |
| `-1` | Deploy chain (DOWN) |

#### Retrieve Chain
```json
{"path": "navigation.anchor.manualControl", "value": 1}
```

#### Stop
```json
{"path": "navigation.anchor.manualControl", "value": 0}
```

#### Deploy Chain
```json
{"path": "navigation.anchor.manualControl", "value": -1}
```

**Note:** SignalK manual commands are blocked when automatic mode is enabled.

#### Physical Remote Control
The system supports a hardwired remote control for direct manual operation.

- **Operation**: The winch is active only as long as the corresponding UP or DOWN button is held down. Releasing the button stops the winch.
- **Priority**: The physical remote overrides any active SignalK manual command.
- **Safety**: The remote is disabled when the system is in automatic mode. The anchor home safety feature remains active, preventing over-retrieval.

The home sensor automatically stops retrieval at home position.

### Reset Counter
```json
{"path": "navigation.anchor.resetRode", "value": true}
```

Resets counter to zero, stops windlass, and clears target. The counter also auto-resets when anchor reaches home position.

## SignalK REST API Examples

### Read Current Chain Length
```bash
curl http://signalk:3000/signalk/v1/api/vessels/self/navigation/anchor/currentRode
```

### Arm and Fire (Automatic Deployment)
```bash
# Arm target to 20m
curl -X POST http://signalk:3000/signalk/v1/api/vessels/self \
  -H "Content-Type: application/json" \
  -d '{"updates":[{"values":[{"path":"navigation.anchor.targetRodeCommand","value":20.0}]}]}'

# Fire (enable automatic mode)
curl -X POST http://signalk:3000/signalk/v1/api/vessels/self \
  -H "Content-Type: application/json" \
  -d '{"updates":[{"values":[{"path":"navigation.anchor.automaticModeCommand","value":1.0}]}]}'
```

### Manual Control
```bash
# UP
curl -X POST http://signalk:3000/signalk/v1/api/vessels/self \
  -H "Content-Type: application/json" \
  -d '{"updates":[{"values":[{"path":"navigation.anchor.manualControl","value":1}]}]}'

# STOP
curl -X POST http://signalk:3000/signalk/v1/api/vessels/self \
  -H "Content-Type: application/json" \
  -d '{"updates":[{"values":[{"path":"navigation.anchor.manualControl","value":0}]}]}'

# DOWN
curl -X POST http://signalk:3000/signalk/v1/api/vessels/self \
  -H "Content-Type: application/json" \
  -d '{"updates":[{"values":[{"path":"navigation.anchor.manualControl","value":-1}]}]}'
```

### Emergency Stop
```bash
curl -X POST http://signalk:3000/signalk/v1/api/vessels/self \
  -H "Content-Type: application/json" \
  -d '{"updates":[{"values":[{"path":"navigation.anchor.automaticModeCommand","value":0.0}]}]}'
```

## Node-RED Integration

### Dashboard Components

**Preset Target Buttons** (recommended for arm and fire operation)
```
[Button: 5m]  [Button: 10m]  [Button: 15m]  [Button: 20m]
```
Each button sends to `navigation.anchor.targetRodeCommand` with respective value.

**Auto Mode Switch**
```
[Toggle: AUTO MODE]  ← Sends 1.0 (on) or 0.0 (off) to navigation.anchor.automaticModeCommand
```

**Manual Control Buttons** (only when auto mode off)
```
[UP]  [STOP]  [DOWN]
```
Button group sending 1, 0, -1 to `navigation.anchor.manualControl`

**Status Display**
- Gauge showing `navigation.anchor.currentRode`
- Text showing `navigation.anchor.automaticModeStatus`
- Text showing `navigation.anchor.targetRodeStatus`

### Using signalk-send-pathvalue Node

The `signalk-send-pathvalue` node (from @signalk/node-red-embedded-signalk) works well with this system:

```javascript
// Example: Preset target button
msg.payload = {
    "path": "navigation.anchor.targetRodeCommand",
    "value": 15.0
};
return msg;
```

Connect to `signalk-send-pathvalue` node configured for PUT requests.

### Example: Arm and Fire Flow

```
[5m Button] ──┐
[10m Button] ─┼──> [Function: Set Target] ──> [signalk-send-pathvalue]
[15m Button] ─┘

[AUTO Switch] ──> [Function: Auto Mode] ──> [signalk-send-pathvalue]
```

**Function: Set Target**
```javascript
msg.payload = {
    "path": "navigation.anchor.targetRodeCommand",
    "value": parseFloat(msg.payload)
};
return msg;
```

**Function: Auto Mode**
```javascript
msg.payload = {
    "path": "navigation.anchor.automaticModeCommand",
    "value": msg.payload ? 1.0 : 0.0
};
return msg;
```

## Configuration

### Hardware Setup
- **Pulse Input:** GPIO 25 - Connect chain counter pulse sensor
- **Direction Input:** GPIO 26 - HIGH = chain out, LOW = chain in
- **Anchor Home Sensor:** GPIO 33 - Detects when anchor is fully retrieved (active LOW)
- **Winch UP Output:** GPIO 27 - Activates windlass to retrieve chain
- **Winch DOWN Output:** GPIO 14 - Activates windlass to deploy chain
- **Remote UP Input:** GPIO 12 - Manual UP from remote (active HIGH)
- **Remote DOWN Input:** GPIO 13 - Manual DOWN from remote (active HIGH)
- **Remote Func 3 Input:** GPIO 15 - Spare remote input (active HIGH)
- **Remote Func 4 Input:** GPIO 16 - Spare remote input (active HIGH)

### Calibration
The meters per pulse conversion factor (default: 0.1) can be configured via the SensESP web interface at:
```
http://bow-sensors.local/
```

Look for the "Meters per Pulse" configuration item to adjust the value based on your windlass gypsy diameter and pulse sensor.
## Safety Features

1. **Manual mode by default** - System starts in manual mode; automatic control must be explicitly enabled
2. **Arm and fire operation** - Set target first, enable auto mode when ready; prevents accidental deployment
3. **Auto-disable on completion** - Automatic mode turns off when target reached; returns to manual mode
4. **Home position protection** - Cannot retrieve past home position; auto-stops and resets counter
5. **Emergency stop** - Send `automaticModeCommand: 0.0` to immediately stop and disable auto mode
6. **Manual control lockout** - Manual commands blocked when automatic mode active
7. **Status feedback** - All mode changes reflected in status paths for monitoring
8. **Stopping tolerance** - Stops within ±0.2m of target (configurable via meters-per-pulse setting)

## Troubleshooting

| Problem | Check | Solution |
|---------|-------|----------|
| Counter not incrementing | Pulse sensor (GPIO 25) | Verify sensor connection and signal |
| Wrong count direction | Direction sensor (GPIO 26) | HIGH = out, LOW = in; swap if reversed |
| Windlass won't start (auto) | Automatic mode status | Send `automaticModeCommand: 1.0` to enable |
| | Target armed | Send `targetRodeCommand` with valid target |
| | Current position | If already at target, no movement occurs |
| Windlass won't retrieve | Home sensor (GPIO 33) | May already be home; check sensor state |
| | Sensor wiring | Should be LOW at home, HIGH otherwise |
| Auto mode won't enable | Manual control active | Ensure no manual commands pending |
| Counter won't reset | Home sensor | Verify LOW signal when anchor home |
| | Manual reset | Send `resetRode: true` |
| Emergency: Can't stop | Auto mode disable | Send `automaticModeCommand: 0.0` |
| | Physical switches | Use hardwired manual controls |

### Debug Output
Monitor serial output (115200 baud) for status messages:
- Pulse count and direction every 5 seconds
- Auto mode enable/disable events
- Target arm/fire events
- Manual control actions
- Home position detection
