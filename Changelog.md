
# Version 1.3.0
- Heaters are now fully dynamic, up to 10!.
- Configurable wattage per heater.
- Heaters can now be enabled/disabled for boil/mash.
- Schedules have a new flag to set then as boil/mash.
- Support for Buzzer.
- Support for Notifications.
- Speech Notifications via webinterface.
- Added Client/Browser Settings.
- Control Screen stays active while in settings.
- Fullscreen button.

# Upgrade Instructions 1.3.0
- Its recommended to do a full erase or factory reset.

```bash
idf.py flash-erase
``` 

# Version 1.2.0
- Support for Celsius and Fahrenheit.
- PID loopt time is now configurable.
- Factory Reset button in system settings.
- Nicer recovery page.
- Factory Reset button in recovery page.
- Added manifest.json and svg icon.

# Version 1.1.0
- Add ability to scan for wifi networks.
- Make it more clear that wifi is is AP mode or Station.
- Add ability to Change max power.
- Set default power to 15, some boards seem to have issues at 20.
- Added both relative and absolute compensation options for sensors.
- Added options to disable/hide sensors.
- Show if sensor is connected in settings.
- Show sensor last temp in settings.
- Allow sensors to be disabled/hidden.
- Show if sensors are connected in sensor settings.
- Show Temperature in sensor settings.
- Added relative and absolute compensation to sensors.
- Round to 0.1.
- Ability to delete unused sensors.
- Button to rescan for onewire sensors.

# Upgrade Instructions 1.1.0
- Temperature sensor settings have been significantly changed, so it is recommended to do a flash_erase and reflash

```bash
idf.py flash-erase
``` 

- Flash Loader and firmware a normal.


# Version 1.0.0
- Multiple Configurable Maish Schedules
- Configurable PID control
- 1 or 2 heater modes
- Up to 10 One-wire Sensors
- Automatic Stirring / Pumping Intervals
- Temperature logging to MQTT
- OTA Firmware update
