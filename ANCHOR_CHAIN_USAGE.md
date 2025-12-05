# Anchor Chain Counter - SignalK Usage Guide

## Overview
This ESP32-based anchor chain counter measures the deployed anchor rode length and provides automatic windlass control via SignalK.

## SignalK Paths

### Output (Device → SignalK)
- **`navigation.anchor.currentRode`** - Current anchor chain length in meters (updated every 1 second)

### Inputs (SignalK → Device)
- **`navigation.anchor.automaticMode`** - Enable/disable automatic windlass control (boolean)
- **`navigation.anchor.targetRode`** - Set target chain length for automatic control (float, meters)
- **`navigation.anchor.manualUp`** - Manual retrieve control - hold to retrieve (boolean)
- **`navigation.anchor.manualDown`** - Manual deploy control - hold to deploy (boolean)
- **`navigation.anchor.resetRode`** - Reset the counter to zero (boolean)

## Usage Examples

### Monitor Current Chain Length
The device continuously publishes the current rode length to `navigation.anchor.currentRode`. Subscribe to this path in your SignalK dashboard to monitor the chain length in real-time.

### Automatic Windlass Control

#### Enable Automatic Mode
```json
{
  "context": "vessels.self",
  "updates": [{
    "values": [{
      "path": "navigation.anchor.automaticMode",
      "value": true
    }]
  }]
}
```

Automatic mode must be enabled before the windlass will respond to target commands.

#### Deploy 15 meters of chain
```json
{
  "context": "vessels.self",
  "updates": [{
    "values": [{
      "path": "navigation.anchor.targetRode",
      "value": 15.0
    }]
  }]
}
```

The windlass will automatically deploy chain until 15 meters is reached, then stop (only if automatic mode is enabled).

#### Retrieve to 5 meters
```json
{
  "context": "vessels.self",
  "updates": [{
    "values": [{
      "path": "navigation.anchor.targetRode",
      "value": 5.0
    }]
  }]
}
```

The windlass will automatically retrieve chain until 5 meters remains, then stop (only if automatic mode is enabled).

#### Disable Automatic Mode (Stop Windlass)
```json
{
  "context": "vessels.self",
  "updates": [{
    "values": [{
      "path": "navigation.anchor.automaticMode",
      "value": false
    }]
  }]
}
```

Disabling automatic mode immediately stops the windlass.

### Manual Windlass Control

Manual control is only available when automatic mode is **disabled**. This provides remote control of the windlass via SignalK.

#### Retrieve Chain Manually
```json
{
  "context": "vessels.self",
  "updates": [{
    "values": [{
      "path": "navigation.anchor.manualUp",
      "value": true
    }]
  }]
}
```

Send `true` to start retrieving, `false` to stop. The home sensor will automatically stop retrieval if the anchor reaches home position.

#### Deploy Chain Manually
```json
{
  "context": "vessels.self",
  "updates": [{
    "values": [{
      "path": "navigation.anchor.manualDown",
      "value": true
    }]
  }]
}
```

Send `true` to start deploying, `false` to stop.

**Important:** Manual control commands are ignored if automatic mode is enabled. You must disable automatic mode first to use manual control.

### Reset Counter
```json
{
  "context": "vessels.self",
  "updates": [{
    "values": [{
      "path": "navigation.anchor.resetRode",
      "value": true
    }]
  }]
}
```

This resets the counter to zero, stops the windlass, and clears any active target.

**Note:** The counter also automatically resets when the anchor reaches the home position (detected by the home sensor on GPIO 33).

**Note:** The counter also automatically resets when the anchor reaches the home position (detected by the home sensor).

## Manual Operation

### Physical Switch Control
When operating the windlass manually using physical switches, the counter continues to measure and report the chain length accurately. The automatic control will not interfere when automatic mode is disabled.

### SignalK Manual Control
You can also control the windlass remotely via SignalK using `manualUp` and `manualDown` paths. This works like a momentary switch - send `true` to activate, `false` to stop. Manual control is only available when automatic mode is disabled.

**Benefits of SignalK manual control:**
- Remote windlass operation from your boat's displays or apps
- All safety features remain active (home sensor, counter tracking)
- Integration with dashboards and automation systems

**Best Practice:** Ensure automatic mode is disabled (`automaticMode: false`) before using any manual control method.

## Using with SignalK REST API

### GET Current Chain Length
```bash
curl http://your-signalk-server:3000/signalk/v1/api/vessels/self/navigation/anchor/currentRode
```

### PUT Enable Automatic Mode
```bash
curl -X PUT http://your-signalk-server:3000/signalk/v1/api/vessels/self/navigation/anchor/automaticMode \
  -H "Content-Type: application/json" \
  -d '{"value": true}'
```

### PUT Target Chain Length (Deploy 20m)
```bash
curl -X PUT http://your-signalk-server:3000/signalk/v1/api/vessels/self/navigation/anchor/targetRode \
  -H "Content-Type: application/json" \
  -d '{"value": 20.0}'
```

### PUT Disable Automatic Mode (Stop Windlass)
```bash
curl -X PUT http://your-signalk-server:3000/signalk/v1/api/vessels/self/navigation/anchor/automaticMode \
  -H "Content-Type: application/json" \
  -d '{"value": false}'
```

### PUT Manual Control (Retrieve)
```bash
# Start retrieving
curl -X PUT http://your-signalk-server:3000/signalk/v1/api/vessels/self/navigation/anchor/manualUp \
  -H "Content-Type: application/json" \
  -d '{"value": true}'

# Stop
curl -X PUT http://your-signalk-server:3000/signalk/v1/api/vessels/self/navigation/anchor/manualUp \
  -H "Content-Type: application/json" \
  -d '{"value": false}'
```

### PUT Manual Control (Deploy)
```bash
# Start deploying
curl -X PUT http://your-signalk-server:3000/signalk/v1/api/vessels/self/navigation/anchor/manualDown \
  -H "Content-Type: application/json" \
  -d '{"value": true}'

# Stop
curl -X PUT http://your-signalk-server:3000/signalk/v1/api/vessels/self/navigation/anchor/manualDown \
  -H "Content-Type: application/json" \
  -d '{"value": false}'
```

### PUT Reset Counter
```bash
curl -X PUT http://your-signalk-server:3000/signalk/v1/api/vessels/self/navigation/anchor/resetRode \
  -H "Content-Type: application/json" \
  -d '{"value": true}'
```

## Node-RED Integration

Node-RED embedded in SignalK can be used to create dashboards and automation for the anchor windlass control.

### Using SignalK Put Node

To send commands from Node-RED, use the **SignalK Put** node with properly formatted messages:

#### Enable/Disable Automatic Mode
```javascript
// Function node code
msg.payload = {
    "path": "navigation.anchor.automaticMode",
    "value": true  // or false to disable
};
return msg;
```

#### Set Target Rode Length
```javascript
// Function node code
msg.payload = {
    "path": "navigation.anchor.targetRode",
    "value": 15.0  // Target length in meters
};
return msg;
```

#### Manual Control (UP/DOWN)
```javascript
// Manual UP - send true to start, false to stop
msg.payload = {
    "path": "navigation.anchor.manualUp",
    "value": true
};
return msg;

// Manual DOWN - send true to start, false to stop
msg.payload = {
    "path": "navigation.anchor.manualDown",
    "value": true
};
return msg;
```

### Dashboard Example

Create a simple control panel using Node-RED Dashboard nodes:

1. **ui_switch** for Automatic Mode
   - Topic: `navigation.anchor.automaticMode`
   - On Payload: `true`
   - Off Payload: `false`
   - Connect to a function node that formats the message, then to SignalK Put node

2. **ui_numeric** for Target Length
   - Min: 0, Max: 100, Step: 0.5
   - Connect to function node that formats as target rode message

3. **ui_button** (Hold type) for Manual UP
   - Sends `true` on press down, `false` on release
   - Connect to function node that formats as manualUp message

4. **ui_button** (Hold type) for Manual DOWN
   - Sends `true` on press down, `false` on release
   - Connect to function node that formats as manualDown message

5. **ui_gauge** to display current rode length
   - Subscribe to `navigation.anchor.currentRode`

### Complete Function Node Example
```javascript
// This function formats any dashboard input for SignalK Put
var path = msg.topic;  // Should be the SignalK path
var value = msg.payload;

// Convert string booleans if needed
if (value === "true") value = true;
if (value === "false") value = false;

// Format for SignalK Put
msg.payload = {
    "path": path,
    "value": value
};

return msg;
```

Connect this function node output to a **signalk-send-put** node to send the command.

## Configuration

### Hardware Setup
- **Pulse Input:** GPIO 25 - Connect chain counter pulse sensor
- **Direction Input:** GPIO 26 - HIGH = chain out, LOW = chain in
- **Anchor Home Sensor:** GPIO 33 - Detects when anchor is fully retrieved (active LOW)
- **Winch UP Output:** GPIO 27 - Activates windlass to retrieve chain
- **Winch DOWN Output:** GPIO 14 - Activates windlass to deploy chain

### Calibration
The meters per pulse conversion factor (default: 0.1) can be configured via the SensESP web interface at:
```
http://bow-sensors.local/
```

Look for the "Meters per Pulse" configuration item to adjust the value based on your windlass gypsy diameter and pulse sensor.
## Safety Notes

1. **System starts in manual mode** - on boot/restart, automatic mode is disabled by default
2. **Always test in safe conditions** before relying on automatic control
3. **Keep manual override accessible** - physical switches and SignalK manual control work when automatic mode is disabled
4. **Monitor the process** - automatic control stops when target is reached (±0.2m tolerance)
5. **Emergency stop** - send `automaticMode: false` to stop automatic operation immediately
6. **Automatic home detection** - the system automatically stops and resets when anchor reaches home position
7. **Enable automatic mode only when ready** - the windlass will not move automatically unless explicitly enabled
8. **Home sensor prevents over-retrieval** - windlass cannot retrieve past the home position, preventing damage

## Troubleshooting

### Counter not incrementing
- Check pulse sensor connection on GPIO 25
- Verify direction sensor on GPIO 26 shows correct state

### Windlass not responding to automatic control
- **Verify automatic mode is enabled** (`automaticMode: true`)
- Check that a valid target was set (`targetRode >= 0`)
- Verify GPIO 27 (UP) and GPIO 14 (DOWN) connections
- Ensure no manual control is active

### Windlass won't retrieve (UP)
- Check if anchor home sensor (GPIO 33) is activated - system prevents retrieval when anchor is already home
- Verify home sensor wiring and function

### Counter doesn't reset at home position
- Verify home sensor connection on GPIO 33
- Sensor should be LOW when anchor is in home position, HIGH otherwise
- Check sensor is triggering correctly (use debug output)

### Wrong direction
- Check direction sensor polarity on GPIO 26
- Swap if needed: HIGH should = chain out, LOW = chain in
