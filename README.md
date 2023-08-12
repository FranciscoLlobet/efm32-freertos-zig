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
- provide a basic MQTT service client (QOS0 and QOS1 support),
- mbedTLS PSK and x509 certificate authentication tested with DTLS,
- Watchdog

## Non-Features

For design, and legal reasons, the following hardware capabilities of the XDK110 will not be supported:

- BLE
- Light Sensor
- Acoustic sensor / Microphone AKU340
- Other IP protocols

## Project Setup

### Zig

Download and install the Zig Compiler

- [Zig Getting-Started](https://ziglang.org/learn/getting-started/)
- [Download Zig](https://ziglang.org/download/)

#### Installation Hints

> This software has been tested using the [Windows x86-64Bit 0.11.0](https://ziglang.org/builds/zig-windows-x86_64-0.11.0-dev.4406+d370005d3.zip) build.

#### Install the Arm GNU Toolchain

- [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

### Code Checkout

```powershell
git clone https://github.com/FranciscoLlobet/efm32-freertos-zig.git
cd .\efm32-freertos-zig\
git submodule init
git submodule update
```

### Build and deploy

```powershell
zig build
zig objcopy -O binary .\zig-out\bin\XDK110.elf .\zig-out\bin\XDK110.bin 
zig objcopy -O hex .\zig-out\bin\XDK110.elf .\zig-out\bin\XDK110.hex 
```

## Used 3rd party software

### Base Tooling

- [MicroZig](https://github.com/ZigEmbeddedGroup/microzig)
- [regz](https://github.com/ZigEmbeddedGroup/regz)

### MCAL and MCU peripheral services

- [EFM32 Gecko SDK](https://github.com/SiliconLabs/gecko_sdk)

### RTOS

- [FreeRTOS](https://github.com/FreeRTOS/FreeRTOS-Kernel)

### Wifi Connectivity

- TI Simplelink CC3100-SDK

### Filesystem

- [FatFs 15](http://elm-chan.org/fsw/ff/00index_e.html). The "standard" FatFS driver for embedded devices.

### Utils

- [jsmn](https://github.com/zserge/jsmn). Jsmn, a very simple JSON parser.

### Crypto and (D)TLS provider

- [mbedTLS](https://github.com/Mbed-TLS/mbedtls)

### Protocol Providers

- [Paho MQTT Embedded-C](https://github.com/eclipse/paho.mqtt.embedded-c). MQTT v3.1.1 protocol service.
- [Wakaama](https://github.com/eclipse/wakaama). LWM2M protocol service.
- [PicoHTTParser](https://github.com/h2o/picohttpparser). Tiny HTTP request and response parser.

### Bosch SensorTec Sensor APIs

- BMA2-Sensor-API
- BME280_driver
- BMG160_driver
- BMI160_driver
- BMM150-Sensor-API

## Configuration fields

Configuration can be loaded via SD card by `config.txt`


```json
{
    "wifi":{
        "ssid":"WIFI_SSID",
        "key":"WIFI_KEY"
    },
    "lwm2m":{
        "device":"LWM2M_DEVICE_NAME",
        "uri":"coaps://leshan.eclipseprojects.io:5684",
        "psk":{
            "id":"LWM2M_PSK_ID",
            "key":"LWM2M_PSK_KEY"
        },
        "bootstrap":false,
        "server_cert":"LWM2M_SERVER_CERT"
    },
    "ntp":{
        "url":[
            "0.de.pool.ntp.org",
            "1.de.pool.ntp.org"
        ]
    },
    "mqtt":{
        "url":"mqtt://localhost:1883",
        "device":"MQTT_DEVICE_ID",
        "username": "MQTT_USER_NAME",
        "password": "MQTT_PASSWORD",
        "psk" : {
            "id": "MQTT_PSK_ID",
            "key":"MQTT_PSK_KEY"
        },
        "cert" : {
            "client" : "MQTT_CERT",
            "server_ca" : "MQTT_SERVER_CA",
        }
    }
}
```
