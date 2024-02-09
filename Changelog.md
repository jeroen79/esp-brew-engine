
# Version 1.2.0
- Support for Celcius and Fahrenheit.
- PID loopt time is now configurable.

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
idf.py flash_erase
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
