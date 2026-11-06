# V1 Web App (Send-Only)

This is the earliest web app version. It displays live sensor telemetry from the ESP32 but cannot send commands back to the device.

**Tradeoff:** Lowest latency (no listeners), but read-only.

**For Back-Functionality:** See `../v2_back_functionality/` which adds bidirectional control.
