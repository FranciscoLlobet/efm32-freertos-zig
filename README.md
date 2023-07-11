# MISO

`MISO` is an example application that combines the power of the [Zig programming language](http://www.ziglang.org) together with a curated selection of services supported by the XDK110 sensor.

## Disclaimers

`MISO` is not affiliated with LEGIC Identsystems Ltd, the LEGIC XDK Secure Sensor Evaluation Kit, the Rust Foundation or the Rust Project. Furthermore, it's important to note that this codebase should not be used in use cases which have strict safety and/or availability requirements.

## Design Philososphy

Our primary aim with `MISO` is to maximize the use of library functions provided by established and respected sources. We take great care to ensure simplicity, and we focus on seamless integration of third-party libraries. Our goal is to build a potent yet easy-to-navigate system that leverages the potential of its underlying technology stack.

## What's inside?

The current version of `MISO` boasts a set of features, including:

- read configuration files from a SD card,
- connect to a designated WiFi AP,
- get a sNTP timestamp from an internet server, 
- provide a basic secured LWM2M service client with connection management,
- provide a basic MQTT service client,
- mbedTLS PSK and x509 certificate authentication tested with DTLS,
- Watchdog

## Non-Features

For design, and legal reasons, the following hardware capabilities of the XDK110 will not be supported:

- BLE
- Light Sensor
- Acoustic sensor / Microphone AKU340
- Other IP protocols

# Used 3rd party software

## Base Tooling

- [MicroZig](https://github.com/ZigEmbeddedGroup/microzig)
- [regz](https://github.com/ZigEmbeddedGroup/regz)

## MCAL and MCU peripheral services

- EFM32 Gecko SDK

## RTOS

- [FreeRTOS](https://github.com/FreeRTOS/FreeRTOS-Kernel)

## Wifi Connectivity

- TI Simplelink CC2100-SDK

## Filesystem

- [FatFs 15](http://elm-chan.org/fsw/ff/00index_e.html). The "standard" FatFS driver for embedded devices.

## Utils

- [jsmn](https://github.com/zserge/jsmn). Jsmn, a very simple JSON parser.

## Crypto and DTLS provider

- [mbedTLS](https://github.com/Mbed-TLS/mbedtls)

## Protocol Providers

- [MQTT-C](https://github.com/LiamBindle/MQTT-C). MQTT protocol service.
- [Wakaama](https://github.com/eclipse/wakaama). LWM2M protocol service.

Bosch SensorTec Sensor APIs:

- BMA2-Sensor-API
- BME280_driver
- BMG160_driver
- BMI160_driver
- BMM150-Sensor-API