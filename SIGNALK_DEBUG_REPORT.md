# SignalK Status Output Debug Report

## System Scope

This report covers SignalK integration for both:
- **Anchor Windlass System**: Chain deployment/retrieval with automatic positioning
- **Bow Thruster System**: Port/starboard directional control
- **Emergency Stop**: Unified safety system affecting both subsystems

---

## Issues Found

### 1. **Emergency Stop Status Never Updates in SignalK**

**Location**: [src/services/SignalKService.cpp](src/services/SignalKService.cpp#L59-L87)

**Problem**: 
- Emergency stop status is set only during emergency_cmd_listener processing (line 70-75)
- **Missing**: No update when emergency stop state changes from other sources (e.g., physical remote double-press)
- The `emergency_stop_status_value_` is created but never explicitly updated when emergency stop service state changes
- Result: SignalK shows stale value; doesn't reflect actual system state

**Root Cause**:
```cpp
// Emergency stop only updates if command comes via SignalK
// But if emergency stop activated by physical remote (RemoteControl), SignalK never updates
if (emergency_stop_service_) {
    emergency_stop_service_->setActive(emergency_active, "signalk");
}
// Missing: No listener on emergency_stop_service_ state changes
```

**Fix Needed**:
- Subscribe to `emergency_stop_service_` state changes
- Update `emergency_stop_status_value_` whenever service state changes
- Use callback or polling (every 100ms) to sync status back to SignalK

---

### 2. **Manual Control Status Not Updated When Motor Stops**

**Location**: [src/services/SignalKService.cpp](src/services/SignalKService.cpp#L89-L115)

**Problem**:
- `manual_control_output_` updated only when command received via SignalK listener
- **Missing**: No update when motor stops due to:
  - Home sensor blocking (anchor)
  - Emergency stop activated
  - Physical remote released
  - Mutual exclusion (bow thruster switched directions)
- Result: SignalK shows last command even though motor stopped

**Fix Needed**:
- Subscribe to anchor winch controller state changes
- Subscribe to bow thruster controller state changes
- Update control status outputs to 0 (STOP) when motors become inactive

---

### 3. **Automatic Mode Status Not Synced from Controller**

**Location**: [src/services/SignalKService.cpp](src/services/SignalKService.cpp#L117-L160)

**Problem**:
- `auto_mode_output_` updated only in listener (when command received via SignalK)
- **Missing**: No update when auto mode disables due to:
  - Manual command overriding
  - Target reached
  - Emergency stop
- Result: SignalK may show "auto enabled" when it's actually disabled

**Fix Needed**:
- Poll `auto_mode_controller_->isEnabled()` periodically
- Update `auto_mode_output_` to stay in sync

---

### 4. **Bow Thruster Status Not Updated When Motor Stops**

**Location**: Implementation omission for bow thruster status

**Problem**:
- Bow thruster status only emitted when command received via SignalK
- **Missing**: No update when motor stops due to:
  - Emergency stop activation
  - Physical remote button release (deadman switch)
  - Direction change (mutual exclusion affects previous direction)
- Result: SignalK shows outdated thruster state (not reflecting actual motor status)

**Affected Path**: `propulsion.bowThruster.status`

**Fix Needed**:
- Add listener to BowPropellerController state changes
- Update `bow_thruster_status_value_` whenever motor state changes
- Ensure status reflects actual direction: 1=STARBOARD, 0=STOP, -1=PORT

---

### 5. **Rode Length Update Only Periodic (Every 1 Second)**

**Location**: [src/services/SignalKService.cpp](src/services/SignalKService.cpp#L35-L37)

**Problem**:
- Rode length only emitted every 1000ms via `event_loop()->onRepeat()`
- **Issue**: Slow feedback when pulses arrive (could arrive immediately, but SignalK update delayed up to 1s)
- For fast windlass, this creates 1-second latency before SignalK reflects chain movement

**Fix Needed** (optional, depends on requirements):
- Option 1: Increase frequency (e.g., 100ms) for faster feedback
- Option 2: Emit on pulse (event-driven) instead of periodic
- Option 3: Keep periodic but reduce interval

---

### 6. **Target Rode Status Not Cleared After Target Reached**

**Location**: [src/services/SignalKService.cpp](src/services/SignalKService.cpp#L165-L192)

**Problem**:
- `target_output_` set when target armed (line 175-176)
- **Missing**: Not cleared when target reached (automatic mode finishes)
- Result: Old target value remains in SignalK even though automation complete

**Fix Needed**:
- When auto mode disables at target, clear target_output_ to -1.0f (no target)
- Or publish new target value (0.0f) to indicate target achieved

---

## Summary: Status Output Sync Issues

| System | Output | Path | Current Status | Issue |
|--------|--------|------|---|---|
| Anchor | Rode Length | navigation.anchor.currentRode | Periodic (1s) | Slow feedback |
| Anchor | Emergency Stop | navigation.bow.ecu.emergencyStopStatus | Listener only | Doesn't sync from service |
| Anchor | Manual Control | navigation.anchor.manualControlStatus | Listener only | Not cleared when motor stops |
| Anchor | Auto Mode | navigation.anchor.automaticModeStatus | Listener only | Not synced from controller |
| Anchor | Target Rode | navigation.anchor.targetRodeStatus | Listener only | Not cleared on completion |
| Bow | Thruster Status | propulsion.bowThruster.status | Listener only | Not updated on motor stop |

---

## Recommended Fix Strategy

### Immediate (High Impact):
1. **Emergency Stop**: Poll service state in `startConnectionMonitoring()` (alongside WiFi check)
2. **Windlass Manual Control**: Add listener to AnchorWinchController state changes
3. **Bow Thruster Status**: Add listener to BowPropellerController state changes
4. **Auto Mode**: Poll controller state in main monitoring loop

### Nice-to-Have:
5. Increase rode length update frequency (if responsiveness needed)
6. Clear target when auto mode completes

---

## Code Locations to Modify

1. **src/services/SignalKService.h**: 
   - Add `bow_thruster_status_value_` if not present
   - May need callbacks in dependencies

2. **src/services/SignalKService.cpp**:
   - `setupEmergencyStopBindings()` - add service state polling
   - `setupManualControlBindings()` - add winch motor state listener
   - `setupBowPropellerBindings()` - add thruster motor state listener and status updates
   - `setupAutoModeBindings()` - track controller state
   - `startConnectionMonitoring()` - expand monitoring loop

3. **src/services/EmergencyStopService.h/cpp**: May need state change callback
4. **src/winch_controller.h/cpp**: May need state change callback
5. **src/bow_propeller_controller.h/cpp**: May need state change callback


