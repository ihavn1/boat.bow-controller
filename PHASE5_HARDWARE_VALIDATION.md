# Phase 5: Hardware Integration Validation

**Status**: 🚀 **STARTING**  
**Objective**: Validate all features on actual ESP32 hardware with comprehensive manual testing  
**Prerequisites**: Phase 4 (unit tests) complete, firmware successfully uploaded to device

---

## 1. Pre-Hardware Checklist

### Physical Setup
- [ ] ESP32 Dev Kit connected via USB to PC
- [ ] Serial monitor ready (COM port identified)
- [ ] Pulse counter test setup ready (simulated chain movement)
- [ ] Remote control buttons accessible
- [ ] SignalK server running on network (if available)
- [ ] Spare GPIO outputs connected for testing

### Firmware Status
- [ ] Latest firmware built and uploaded
- [ ] Device boots without errors
- [ ] Serial logs show initialization complete
- [ ] No runtime crashes or watchdog resets

---

## 2. Manual Feature Testing

### 2.1 Hardware Initialization (Critical Safety)

**Objective**: Verify device boots safely with all relays inactive

**Test Steps**:
1. Connect ESP32 via USB
2. Open serial monitor at 115200 baud
3. Observe boot sequence in logs
4. Check GPIO pin states (use multimeter or logic analyzer):
   - WINCH_UP (GPIO 13): Should read HIGH (inactive)
   - WINCH_DOWN (GPIO 12): Should read HIGH (inactive)
   - REMOTE_OUT1 (GPIO 4): Should read HIGH (inactive)
   - REMOTE_OUT2 (GPIO 5): Should read HIGH (inactive)

**Success Criteria**:
- ✅ Device boots without errors
- ✅ All relay pins read HIGH on boot
- ✅ No watchdog resets detected
- ✅ Logs show `"=== Boat Anchor Chain Counter System ==="`

**Commands** (if available via SignalK):
```
Send: GET /navigation/anchor/status
Expected: Safe state, all outputs inactive
```

---

### 2.2 Pulse Counter & Rode Length Calculation

**Objective**: Verify pulse counting accuracy and rode length calculation

**Test Equipment**:
- Function generator (or manual pulse simulation via GPIO)
- Pulse frequency: 1-10 Hz (simulates chain moving at 1-10 m/min)

**Test Procedure**:

#### Test 2.2.1: Forward Counting (Chain Out)
```
Setup:
  - DIRECTION pin (GPIO 26) = HIGH (chain out)
  - PULSE_INPUT pin (GPIO 35) = toggle at 2 Hz for 10 seconds (20 pulses)
  - Meters per pulse configured as: 0.01m (from config)

Expected:
  - Pulse count increases by 20
  - Rode length increases by 0.2m (20 pulses × 0.01m/pulse)
  - SignalK output: navigation.anchor.rodeLength = 0.2

Verify via logs:
  - Serial should show: "Pulse count: 20, Rode: 0.20m"
  - SignalK output shows matching value
```

#### Test 2.2.2: Reverse Counting (Chain In)
```
Setup:
  - DIRECTION pin (GPIO 26) = LOW (chain in)
  - PULSE_INPUT pin (GPIO 35) = toggle at 2 Hz for 10 seconds (20 pulses)

Expected:
  - Pulse count decreases by 20
  - Rode length decreases by 0.2m
  - Cannot go negative (clamped to 0)
```

#### Test 2.2.3: Home Position Protection
```
Setup:
  - Pulse count at 5.0m (500 pulses)
  - DIRECTION = HIGH (chain out)
  - Home sensor: NOT triggered

Results:
  - Can retrieve chain (count decreases)
  - Rode length goes down correctly

Then:
  - Simulate home sensor triggered (ANCHOR_HOME pin = LOW)
  - Try to extend chain via command

Expected:
  - Motor should not activate (blocked by home sensor)
  - Logs show: "Home sensor blocking WINCH_UP"
  - Rode length stays constant
```

**Success Criteria**:
- ✅ Pulse counts match input frequency
- ✅ Rode length accurate ±0.02m over 1m range
- ✅ Negative counts prevented (clamped to 0)
- ✅ Home sensor blocks deployment

**Commands**:
```
# Check rode length via SignalK
GET /navigation/anchor/rodeLength
```

---

### 2.3 Manual Control (UP/DOWN/STOP)

**Objective**: Verify motor control responds to commands

**Test Setup**:
- Motor GPIO pins monitored (logic analyzer recommended)
- WINCH_UP: GPIO 13
- WINCH_DOWN: GPIO 12

**Test 2.3.1: Manual UP Command**
```
Command via SignalK:
  PUT /navigation/anchor/manualControl = 1

Expected:
  - WINCH_UP (GPIO 13) = LOW (within 100ms)
  - WINCH_DOWN (GPIO 12) = HIGH (stays inactive)
  - Serial log: "Manual control: UP"
  - Device continues until STOP command

Timing:
  - Activation latency: < 50ms (SensESP event loop)
```

**Test 2.3.2: Manual DOWN Command**
```
Command via SignalK:
  PUT /navigation/anchor/manualControl = -1

Expected:
  - WINCH_DOWN (GPIO 12) = LOW (within 100ms)
  - WINCH_UP (GPIO 13) = HIGH (stays inactive)
  - Serial log: "Manual control: DOWN"
```

**Test 2.3.3: Manual STOP Command**
```
Command via SignalK:
  PUT /navigation/anchor/manualControl = 0

Expected:
  - Both WINCH_UP and WINCH_DOWN = HIGH (inactive)
  - Serial log: "Manual control: STOP"
  - Motor stops immediately
```

**Test 2.3.4: Control Override**
```
Setup:
  - Send UP command (motor active)
  - While motor running, send DOWN command

Expected:
  - UP signal drops (deactivates)
  - DOWN signal activates
  - No intermediate "both HIGH" window > 10ms (prevent motor jerk)
  - Mode switches smoothly without glitches
```

**Success Criteria**:
- ✅ Commands activate correct relay pins
- ✅ Timing < 100ms from command to GPIO change
- ✅ No "floating" states (both pins high/low simultaneously)
- ✅ Smooth transitions between modes

**Timing Goals**:
```
Command → Parsing: 5-10ms
Parsing → GPIO change: 20-50ms
Total latency: < 100ms
```

---

### 2.4 Emergency Stop

**Objective**: Verify emergency stop activates and prevents control

**Test 2.4.1: Emergency Stop Activation**
```
Setup:
  - Send UP command (motor active)
  - While motor running, activate emergency stop

Command (SignalK):
  PUT /navigation/anchor/emergencyStop = 1

Expected Immediately:
  - Both WINCH_UP and WINCH_DOWN → HIGH (motor stops)
  - Serial log: "EMERGENCY STOP ACTIVATED (signalk)"
  - SignalK output: navigation.anchor.emergencyStop = 1
  - All control inputs blocked

Timing:
  - Motor stop latency: < 100ms
  - Relay deactivation: immediate
```

**Test 2.4.2: Control Blocked During Emergency Stop**
```
Setup:
  - Emergency stop is active
  - Try to send UP/DOWN/manual commands

Expected:
  - Motor doesn't activate
  - Relays stay HIGH (inactive)
  - SignalK command returns: command rejected (internal logic)
  - Logs show: "Emergency stop active - blocking command"
```

**Test 2.4.3: Clear Emergency Stop**
```
Command (SignalK):
  PUT /navigation/anchor/emergencyStop = 0

Expected:
  - signalk output: navigation.anchor.emergencyStop = 0
  - Control inputs accepted again
  - Motor can be commanded

Timing:
  - Response < 100ms
```

**Success Criteria**:
- ✅ Emergency stop activates immediately
- ✅ Motor stops within 100ms
- ✅ All control blocked while active
- ✅ Clear command restores control
- ✅ No "stuck" states (watchdog resets if needed)

---

### 2.5 Physical Remote Control (If Equipped)

**Objective**: Verify physical buttons override SignalK

**Hardware Required**:
- Remote control buttons wired to GPIO (see pin_config.h)

**Test 2.5.1: Physical UP Button**
```
Press physical UP button

Expected:
  - WINCH_UP → LOW within 10ms
  - Motor activates
  - SignalK manual control status updates to 1
  - Works even if SignalK sends STOP command
```

**Test 2.5.2: Physical DOWN Button**
```
Press physical DOWN button

Expected:
  - WINCH_DOWN → LOW within 10ms
  - Motor activates
  - SignalK status updates to -1
```

**Test 2.5.3: Button Release Stops Motor**
```
Press and hold UP button
Release button

Expected:
  - Motor stops immediately on release
  - Both relays → HIGH
  - Logs show: "Physical remote released - STOP"
  - Even if SignalK still sending UP command, stop takes priority
```

**Test 2.5.4: Physical Overrides SignalK**
```
Setup:
  - Send UP via SignalK (motor running)
  - Press DOWN button

Expected:
  - Motor switches to DOWN mode
  - Physical input takes priority
  - Logs show: "Physical remote overriding SignalK"
```

**Success Criteria**:
- ✅ Buttons responsive (< 50ms latency)
- ✅ Release stops motor immediately
- ✅ Override behavior works correctly
- ✅ No conflicting signals to motor

---

### 2.6 Automatic Mode Test

**Objective**: Verify automatic deployment/retrieval to target

**Test 2.6.1: Arm Target**
```
Command (SignalK):
  PUT /navigation/anchor/targetRodeCommand = 5.0

Expected:
  - SignalK output: navigation.anchor.targetRodeStatus = 5.0
  - Logs show: "Target armed: 5.0m"
  - Motor does NOT activate (not yet fired)
  - Current rode: 2.0m → Target: 5.0m

Timing:
  - Acknowledge within 50ms
```

**Test 2.6.2: Fire Deploy (Below Target)**
```
Prerequisites:
  - Target armed at 5.0m
  - Current rode: 2.0m

Command (SignalK):
  PUT /navigation/anchor/automaticModeCommand = 1.0

Expected:
  - WINCH_UP → LOW (deploy chain)
  - Motor activates and runs  
  - Pulse counter increases rode length
  - Logs show: "Automatic mode: DEPLOYING to 5.0m"

Continue until:
  - Rode length reaches 5.0m ± tolerance (0.02m)
  - WINCH_UP → HIGH (motor stops)
  - SignalK output: navigation.anchor.automaticModeStatus = 0 (auto-disabled)
  - Logs show: "Target reached, auto-mode disabled"

Timing:
  - Deployment rate: ~1m per 60+ pulses (depends on motor/chain speed)
```

**Test 2.6.3: Fire Retrieve (Above Target)**
```
Prerequisites:
  - Current rode: 8.0m
  - Target armed at 5.0m

Command (SignalK):
  PUT /navigation/anchor/automaticModeCommand = 1.0

Expected:
  - WINCH_DOWN → LOW (retrieve chain)
  - Motor activates and runs
  - Pulse counter decreases rode length
  - Logs show: "Automatic mode: RETRIEVING to 5.0m"

Continue until:
  - Rode length reaches 5.0m ± tolerance
  - WINCH_DOWN → HIGH (motor stops)
  - SignalK output: navigation.anchor.automaticModeStatus = 0
```

**Test 2.6.4: Automatic Mode Blocked at Home**
```
Prerequisites:
  - Home sensor near
  - Current rode: 1.5m
  - Target armed at 2.0m

If ANCHOR_HOME pin triggered during deploy:
  - WINCH_UP → HIGH (motor stops)
  - SignalK shows: "Home sensor blocking deployment"
  - Automatic mode disables
  - Rode length stops changing
```

**Test 2.6.5: Manual Override Auto Mode**
```
Prerequisites:
  - Automatic mode running (motor active)
  - Deploying to target

Send manual command (SignalK):
  PUT /navigation/anchor/manualControl = -1

Expected:
  - Automatic mode disables immediately
  - Motor switches to DOWN mode (manual command)
  - Logs show: "Manual control overriding automatic mode"
  - Auto deployment stopped
```

**Success Criteria**:
- ✅ Target arms without activating motor
- ✅ Deploy reaches target ±0.02m
- ✅ Retrieve reaches target ±0.02m
- ✅ Stops exactly at tolerance
- ✅ Blocked by home sensor
- ✅ Manual override works

---

### 2.7 SignalK Connection & Status

**Objective**: Verify device connects to SignalK server and publishes status

**Prerequisites**:
- SignalK server running on network (Node-RED or SignalK server)
- Device on same WiFi network
- Network WiFi credentials in secrets.h

**Test 2.7.1: WiFi Connection**
```
Expected in logs:
  - "WiFi connecting to: [SSID]"
  - "WiFi connected!"
  - "IP: 192.168.x.x"
```

**Test 2.7.2: SignalK Connection**
```
Expected in logs:
  - "Connecting to SignalK server..."
  - "SignalK connection established"
  - "Publishing: navigation.anchor.rodeLength = X"
```

**Test 2.7.3: Status Outputs**
```
Check SignalK server for values:
  - navigation.anchor.rodeLength (rode deployed, meters)
  - navigation.anchor.emergencyStop (0/1 boolean)
  - navigation.anchor.manualControlStatus (current manual state)
  - navigation.anchor.automaticModeStatus (auto mode running)
  - navigation.anchor.targetRodeStatus (armed target)
```

**Test 2.7.4: Input Listeners**
```
Send values from SignalK server:
  - navigation.anchor.manualControl = 1 (expect motor UP)
  - navigation.anchor.emergencyStop = 1 (expect motor stops)
  - navigation.anchor.automaticModeCommand = 1.0 (expect auto-mode)
  - navigation.anchor.targetRodeCommand = X (expect target armed)

Verify device responds to each
```

**Test 2.7.5: Connection Loss Recovery**
```
Setup:
  - Device connected and communicating
  - SignalK server running

Disconnect WiFi (reboot server or change password)
  - Device should detect disconnect
  - No crashes or infinite loops
  - Logs show reconnection attempts

Reconnect WiFi
  - Auto-reconnect succeeds
  - Status resumes publishing
  - Commands accepted again
```

**Success Criteria**:
- ✅ WiFi connects in < 15 seconds
- ✅ SignalK connects after WiFi
- ✅ All status outputs published
- ✅ All input listeners functional
- ✅ Graceful disconnect/reconnect

---

## 3. Load Testing

### 3.1 Sustained Motor Operation
```
Objective: Motor stability under continuous operation

Test:
  - Activate motor at 50% duty (pulse command every 50ms)
  - Run for 5 minutes continuous
  - Monitor:
    - Temperature (device shouldn't get hot)
    - Current draw (relay amperage)
    - Watchdog resets (should be zero)
    - Serial logs (should be smooth)

Success:
  - ✅ No watchdog resets
  - ✅ No temperature warnings
  - ✅ Smooth operation throughout
```

### 3.2 Rapid Command Switching
```
Objective: Handle fast mode changes without glitches

Test:
  - Send UP command
  - After 100ms, send STOP
  - After 100ms, send DOWN
  - After 100ms, send STOP
  - Repeat 10 times in rapid succession

Success:
  - ✅ All transitions smooth
  - ✅ No stuck states
  - ✅ GPIO changes correct
  - ✅ No motor "jerks" or strange behavior
```

### 3.3 High Pulse Rate
```
Objective: ISR handling at high frequencies

Setup:
  - Pulse input at 100 Hz (10 meter/sec equivalent)
  - Direction alternating

Expected:
  - All pulses counted correctly
  - No dropped counts
  - No overflow issues
  - Rode length accurate
```

---

## 4. Safety Validation

### 4.1 Relay Defaults
```
Test:
  - Power cycle device 10 times
  - Measure relay GPIO pins on each boot

Expected:
  - ✅ All relays HIGH (inactive) on every boot
  - ✅ No momentary activation
  - ✅ Safe by default always
```

### 4.2 Watchdog/Crash Recovery
```
Test:
  - Monitor for watchdog resets (logs should show)
  - If watchdog triggers, device reboots to safe state
  - Verify recovery: all relays inactive, system responsive

Expected:
  - ✅ No unnecessary resets during normal operation
  - ✅ Recovery is clean and safe
  - ✅ No data corruption
```

### 4.3 Power Supply Isolation
```
Test (if relay module isolated):
  - Monitor current draw under different conditions
  - Peak current: motor active + all outputs
  - Sustained current: idle with WiFi connected

Expected:
  - ✅ USB power sufficient for operation
  - ✅ No voltage sag on device
  - ✅ Relay isolation working (galvanic separation)
```

---

## 5. Performance Metrics

### 5.1 Timing Measurements

| Metric | Target | Tolerance |
|--------|--------|-----------|
| WiFi connection | 10-15s | < 20s OK |
| SignalK connect | 5-10s | < 15s OK |
| Manual control latency | < 100ms | < 200ms acceptable |
| Emergency stop latency | < 100ms | < 200ms critical |
| Pulse ISR response | < 100µs | < 1000µs OK |
| Rode length update | < 100ms | < 500ms OK |

### 5.2 Memory Usage

```
Expected (from Phase 3):
  - Flash: ~70% (69.8 KB / ~100 KB available)
  - RAM: ~12% (30+ KB / 250+ KB available)

Monitor during runtime:
  - No progressive RAM loss (heap fragmentation)
  - No stack overflows
  - Stable operation > 1 hour
```

### 5.3 Power Consumption

| State | Current |  Notes |
|-------|---------|--------|
| Idle (WiFi connected) | 40-60 mA | Normal operation |
| Motor UP | ~200-300 mA | Depends on motor load |
| Motor DOWN | ~200-300 mA | May vary by load |
| WiFi searching | 200+ mA | Higher power during connect |

---

## 6. Troubleshooting Guide

### Issue: Device boots but motor won't respond to commands

**Possible Causes**:
1. Relay module not powered (check external 12V supply)
2. Motor GPIO pins damaged (test with multimeter)
3. Remote control override active (press release to deactivate)
4. Emergency stop active (send clear command)
5. Home sensor triggering falsely (check wiring)

**Debug Steps**:
```cpp
// Check logs for errors
Serial.println("Debug: Checking motor relay state");
digitalWrite(WINCH_UP, LOW);   // Force relay on
delay(100);
// Use multimeter to verify relay activation
```

### Issue: Pulse count wrong or stuck

**Possible Causes**:
1. PULSE_INPUT pin floating or damaged
2. Direction pin stuck
3. ISR not firing
4. Pull-up resistors weak

**Debug Steps**:
```cpp
// Monitor pulse ISR in logs
debugD("Pulse ISR firing: count=%d", pulse_count);
// Check pin states with logic analyzer
```

### Issue: SignalK won't connect

**Possible Causes**:
1. WiFi not connected (check logs for WiFi status)
2. SignalK server IP/port wrong
3. Device on wrong network
4. Firewall blocking connection

**Debug Steps**:
```
In logs, should see:
  - "WiFi connected: IP 192.168.x.x"
  - "Connecting to SignalK: 192.168.x.x:3000"
  - "SignalK connected"
```

---

## 7. Test Documentation

### For Each Test, Record:
- ✅ Test name and objective
- ✅ Procedure followed
- ✅ Results (pass/fail)
- ✅ Any anomalies or timing issues
- ✅ Device logs (save serial output if test fails)
- ✅ Date and firmware version tested

### Create Test Log File:
```
test_log_2026-02-16.txt:

Test: Manual UP Command
  - Sent command via SignalK
  - Expected: WINCH_UP → LOW
  - Observed: WINCH_UP → LOW at T+45ms
  - Status: ✅ PASS
  
Test: Emergency Stop
  - Sent emergency stop command
  - Expected: All relays → HIGH
  - Observed: Motor stopped, relays HIGH
  - Latency: 87ms
  - Status: ✅ PASS
  
...etc
```

---

## 8. Success Criteria (Phase 5 Complete)

Phase 5 is complete when:

✅ Hardware Initialization
  - Device boots safely every time
  - All relays inactive on start-up
  - No watchdog resets during normal operation

✅ Pulse Counting
  - Accurate within ±0.02m over 1+ meter range
  - Both directions (chain out/in) work
  - Home sensor prevents unsafe states

✅ Manual Motor Control
  - UP/DOWN/STOP commands work via SignalK
  - Motor responds within 100ms
  - Smooth transitions between modes

✅ Emergency Stop
  - Activates immediately (< 100ms)
  - Prevents all control
  - Clear command restores control

✅ Automatic Mode
  - Deploys to target accurately
  - Retrieves to target accurately
  - Respects home sensor limits
  - Manual override works

✅ SignalK Integration
  - Device connects to SignalK server
  - All status inputs/outputs work
  - Commands processed correctly
  - Connection recovers from WiFi loss

✅ Safety
  - All relays default inactive on boot
  - No dangerous floating states
  - Graceful error handling
  - No crashes in 1+ hour sustained operation

✅ Performance
  - Manual control latency < 150ms
  - Emergency stop latency < 100ms
  - Pulse ISR no dropped counts
  - Memory stable (no leaks)

---

## Next Steps

After Phase 5 validation:
- **Phase 6**: Deploy to boat and perform real-world field testing
- Create user documentation and operation manual
- Set up OTA update mechanism for firmware patches
- Monitor system in production for 1+ week before considering "stable"

