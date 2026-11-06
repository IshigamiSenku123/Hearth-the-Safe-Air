THIS V1 Contains the Basic Modularized Firmware with some errors, this is for learning only, so that you may use the same Firmware files of V1 to make your own Smart Devices!
That being said, V1 contains th
## V1 — Send-Only Pipeline
 "working_2nd attempt"		gives the minimal front end ( no backcommands , no solid telemetry link )
  "SafeAir_Firmware" 		gives the template for the project ( might work maybe after optimization )
  "firmware-working"		gives the working code for the project (should work )
   "SafeAir_Integration"	gives the steps on integrating the backend of the app code
   "SafeAir_Backend"		gives the future steps for backend+integration
**Folder:** `v1_send_only/` (contains three subfolders)
**Structure:**
- `backend/` — Firebase Functions skeleton and database schema--------------	only for reference, no guarantee --
- `webapp/` — Read-only web interface (no Back-Functionality)---------------	only for reference, no guarantee --
- `firmware_early/` — Early ESP32 code (send-only, no command listeners)----	only for reference, no guarantee --
**What it could do**
- Send sensor telemetry from device to cloud with minimal latency ( ONLY IF YOU OPTIMIZE it , otherwise it would throw errors)
- Provide backend infrastructure template for future expansion ( that it does) LIKE Firebase Functions skeleton and database schema
- Provide backend integration steps too
- Provide front-end infrastructure template like separating the .html and .json and making a bridging link between them
- Provide front end -Read-only web interface (no Back-Functionality 	ONLY AFTER OPTIMIZATION)
	*SO IN ITS CURRENT STATE IT DOENST GIVE 100% SOLID RESULTS*
**What it gave up**
- **No Back-Functionality** — cannot command the device
- Not a finished product on its own

**Best for:** Understanding the data pipeline, or as a template for building your own backend.
