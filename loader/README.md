
# ESP Brew Engine Loader

## Description

This is the loader for the ESP Brew Engine brew controller.

Its a simple webinterface to update the firmware, the reason for this is that most esp32 only have 4MB of flash and that would be to small for a factory and 2 ota partitions.

## Supported Devices

It should work with pretty much with esp32 controller, but the easyest way is to buy a esp board wich already has 4 relais on it.

## How to use

- Build and flash, or fash the binary release.
- Connect to the wifi EspBrewEngine with password EspBrewEngine123
- Navigate to http://192.168.4.1
- Upload firmware
- Restart Device

## Setup Dev Enviroment

install esp-idf >= 5.1.2

run . ./export.sh in esp-idf folder


## Configure the project

```bash
idf.py fullclean
```

```bash
idf.py set-target esp32   
```
valid targets are: esp32, esp32s2, esp32c3, esp32c6, esp32h2, esp32c2, esp32s3

```bash
idf.py menuconfig 
```

Go into "BrewEngine Config" and configure wifi if needed.



## Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```bash
idf.py build flash
```

*Depending on your device you might need to set it into flash mode, typically: Hold both the BOOT and RESET, then release RESET.

## Wifi

To configure wifi either do idf.py menuconfig, and configure wifi SSID and password, or let esp-brew-engine start an access point wich you can connect to and configure wifi in the webinterface.

Be sure to at least change te password!, but it is also recommended to swith to station mode instead of ap!

Default Wifi settings are:
SSID: EspBrewEngine
Password: EspBrewEngine123

## Connecting with Browser

Just open your browser and enter http://BrewEngine, or if still in access point mode http://192.168.4.1

Hostname can also be changed via menuconfig.

*Dns does not work when in access point mode, esp-idf doesn't have a dns server atm. 


## Debug

```bash
idf.py monitor 
```

To quit Ctrl-T Ctrl-X.
