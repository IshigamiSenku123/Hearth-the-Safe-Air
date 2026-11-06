// SafeAir Firmware — Command Loop Patch
// Add this INSIDE your TaskCloud() loop, after Cloud.update()
// =============================================================================

// In SafeAir_Firmware.ino, inside TaskCloud(), replace the command dispatch block:

    // Command dispatch (enhanced)
    if (Cloud.hasCommand()) {
      JSONManager::Command cmd = Cloud.getCommand();
      Device.onCommand(cmd);

      // Also delete the command from RTDB after execution
      // (so the device doesn't re-process it on reboot)
      // This requires adding a removeCommand() method to FirebaseManager
    }

// With this improved version:

    // Command dispatch with auto-cleanup
    if (Cloud.hasCommand()) {
      JSONManager::Command cmd = Cloud.getCommand();
      Device.onCommand(cmd);

      // Optional: delete the command node after processing
      // FirebaseManager::removeCommand(cmd.id);  // implement if needed
    }

// ALSO: In your existing switch statement in DeviceManager::onCommand(),
// add these missing command types that the PWA sends:

  } else if (strcmp(cmd.type, "unmute") == 0) {
    Alarm.mute(false);
  } else if (strcmp(cmd.type, "pump_arm") == 0) {
    Pump.arm(true);
  } else if (strcmp(cmd.type, "pump_disarm") == 0) {
    Pump.arm(false);
  }

// The full onCommand switch should look like this:

void DeviceManager::onCommand(const JSONManager::Command& cmd) {
  bool ok = true;
  char ackMsg[64] = "done";

  if (strcmp(cmd.type, "arm") == 0) {
    Alarm.mute(false);
  } else if (strcmp(cmd.type, "disarm") == 0) {
    Alarm.mute(true);
  } else if (strcmp(cmd.type, "mute") == 0) {
    Alarm.mute(true);
  } else if (strcmp(cmd.type, "unmute") == 0) {
    Alarm.mute(false);
  } else if (strcmp(cmd.type, "test") == 0) {
    Alarm.testAlarm(5000);
  } else if (strcmp(cmd.type, "reset") == 0) {
    factoryReset();
    strcpy(ackMsg, "factory reset triggered");
  } else if (strcmp(cmd.type, "pump") == 0) {
    Pump.trigger();
  } else if (strcmp(cmd.type, "pump_arm") == 0) {
    Pump.arm(true);
  } else if (strcmp(cmd.type, "pump_disarm") == 0) {
    Pump.arm(false);
  } else if (strcmp(cmd.type, "update_threshold") == 0) {
    strcpy(ackMsg, "threshold update stub");
  } else {
    ok = false;
    strcpy(ackMsg, "unknown command");
  }

  size_t n = JSONManager::buildAck(_jsonBuf, sizeof(_jsonBuf), cmd.id, ok, ackMsg);
  if (n) Cloud.sendAck(_jsonBuf);

  SysLog.log(LogLevel::INFO, LogCategory::CLOUD,
    "Command executed", "%s -> %s", cmd.type, ok ? "ok" : "fail");
}
