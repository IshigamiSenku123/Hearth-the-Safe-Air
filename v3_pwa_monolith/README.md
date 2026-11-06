# SafeAir PWA (Monolith Version)

This is the installable Progressive Web App with the Firebase bridge merged into a single HTML file.
 OF course the PWA bridgeing strategy should be implemented for future use, so that the security could be enhanced
## What it does
- Displays live sensor readings from the ESP32
- Sends commands (mute, test, arm/disarm, pump) back to the device
- Installable on phone home screens/ laptops, etc. via PWA manifest

## Tradeoff
The bridge logic is merged into `safeair-app.html`, making it harder to maintain than the modular V4 version. Use this if you want ready-made PWA assets in one place; otherwise prefer `v4_modular_bridge/`.

## Partner component
This web app requires the **ESP32 firmware** to be running and publishing data. See:
- `../firmware_final/main/`
