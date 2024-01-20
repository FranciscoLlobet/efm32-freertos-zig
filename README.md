# MISO

`MISO` is an example application that combines the power of the [Zig programming language](http://www.ziglang.org) together with a curated selection of services supported by the XDK110 sensor.

## Status

[![zig-build](https://github.com/FranciscoLlobet/efm32-freertos-zig/actions/workflows/zig-build.yml/badge.svg)](https://github.com/FranciscoLlobet/efm32-freertos-zig/actions/workflows/zig-build.yml)

> zig-build passing means that a binary build is possible. Further testing on target is necessary.

## Disclaimers

`MISO` is not affiliated with LEGIC Identsystems Ltd, the LEGIC XDK Secure Sensor Evaluation Kit, the Rust Foundation or the Rust Project. Furthermore, it's important to note that this codebase should not be used in use cases which have strict safety and/or availability requirements.

## What's inside?

The current version of `MISO` boasts a set of features, including:

- read configuration files from a SD card,
- provide configuration persistance using non-volatile memory (NVM),
- connect to a designated WiFi AP,
- get a sNTP timestamp from an internet server and run a real-time clock (RTC),
- provide a basic secured LWM2M service client with connection management,
- provide a basic MQTT service client (QOS0 and QOS1 support, QOS2 experimental),
- provide a HTTP file download client for configuration and firmware updates
- mbedTLS PSK and x509 certificate authentication tested with TLS (MQTTS) and DTLS (COAPS),
- Watchdog
- Bootloader with support for MCUBoot firmware containers

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

> This software has been tested using the [Windows x86-64Bit 0.11.0](https://ziglang.org/download/0.11.0/zig-windows-x86_64-0.11.0.zip) and macOS (via brew) builds.

#### Install the Arm GNU Toolchain

> The Arm GNU Toolchain is now optional for building since `MISO`'s move to `picolibc`.

Install the Arm GNU toolchain if debugging the code is required.

https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

### Picolibc

Picolibc is provided precompiled using the clang compiler on Ubuntu WSL.

### Code Checkout

Perform a recursive clone:

```shell
git clone --recursive https://github.com/FranciscoLlobet/efm32-freertos-zig.git
```

Alternatively clone and update submodules:

```shell
git clone https://github.com/FranciscoLlobet/efm32-freertos-zig.git
cd .\efm32-freertos-zig\
git submodule init
git submodule update
```

### Build and deploy

```powershell
zig build
```

## Used 3rd party software

### Base Tooling

- [MicroZig](https://github.com/ZigEmbeddedGroup/microzig)
- [regz](https://github.com/ZigEmbeddedGroup/regz)
- [picolibc](https://keithp.com/picolibc/)

### MCAL and MCU peripheral services

- [EFM32 Gecko SDK](https://github.com/SiliconLabs/gecko_sdk)

### RTOS

- [FreeRTOS](https://github.com/FreeRTOS/FreeRTOS-Kernel)

### Firmware Container

- [MCUBoot](https://github.com/mcu-tools/mcuboot)

### Crypto and (D)TLS provider

- [mbedTLS](https://github.com/Mbed-TLS/mbedtls)

### Wifi Connectivity

- [TI Simplelink CC3100-SDK](https://www.ti.com/tool/CC3100SDK)

### Filesystem

- [FatFs 15](http://elm-chan.org/fsw/ff/00index_e.html).

### Utils

- [jsmn](https://github.com/zserge/jsmn). Jsmn, a very simple JSON parser.

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

| Field          | Description            | Type                  |                      |
|----------------|------------------------|-----------------------|----------------------|
| wifi.ssid      | Wifi WPA2 SSID         | String                | Mandadory            |
| wifi.key       | Wifi WPA2 SSID Key     | String                | Mandatory            |
| lwm2m.endpoint | LWM2M endpoint         | String                | If LwM2M is compiled |
| lwm2m.uri      | LWM2M Server Uri       | URI String            | If LwM2M is compiled |
| lwm2m.psk.id   | LwM2M DTLS Psk ID      | String                | Optional             |
| lwm2m.psk.key  | LwM2M DTLS PSK Key     | Base64 encoded String | Optional             |
| ntp.url[]      | sNTP server URI/URL    | URI String Array      | Not implemented      |
| http.uri       | Firmware image URI     | URI String            | Mandatory            |
| http.key       | Firmware public key    | Base64 encoded string | Mandatory            |

```json
{
  "wifi": {
    "ssid": "WIFI_SSID",
    "key": "WIFI_KEY"
  },
  "lwm2m": {
    "endpoint": "LWM2M_DEVICE_NAME",
    "uri": "coaps://leshan.eclipseprojects.io:5684",
    "psk": {
      "id": "LWM2M_DTLS_PSK_ID",
      "key": "LWM2M_DTLS_PSK_KEY_BASE64"
    },
    "bootstrap": false,
  },
  "ntp": {
    "url": [
      "0.de.pool.ntp.org",
      "1.de.pool.ntp.org"
    ]
  },
  "mqtt": {
    "url": "mqtts://MQTT_BROKER_HOST:8883",
    "device": "MQTT_DEVICE_ID",
    "username": "MQTT_USER_NAME",
    "password": "MQTT_PASSWORD",
    "psk": {
      "id": "MQTT_PSK_TLS_ID",
      "key": "MQTT_PSK_TLS_KEY_BASE64"
    },
    "cert": {
      "client": "MQTT_CERT",
      "server_cert": "MQTT_SERVER_C"
    }
  },
  "http": {
    "url": "http://HTTP_HOST:80/app.bin",
    "key": "APP_PUB_KEY_BASE64"
  }
}
```

## Signature Algorithms

`MISO` uses elliptic curve cryptography for signature creation and validation of configuration and firmware images using the ECDSA (*Eliptic Curve Digital Signature Algorithm*).

Parameters:

- `P-256` Weierstrass curve
  - `prime256v1`(OpenSSL)
  - `secp256r1`(mbedTLS)
- `SHA256` hash digest.

> The algorithms have been tested using OpenSSL on host (MacOS, Win11) and mbedTLS on target (EFM32/XDK110)

## Encryption Algorithms and TLS Cyphersuites

### PSK Cyphersuite

AES-Based Suites

- TLS_PSK_WITH_AES_128_GCM_SHA256
- TLS_PSK_WITH_AES_128_CCM_8
- TLS_PSK_WITH_AES_128_CCM

ECDHE

- TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8
- TLS_ECDHE_ECDSA_WITH_AES_128_CCM

### Eliptic curves

- MBEDTLS_ECP_DP_SECP256R1_ENABLED

### Reference

- <https://wiki.openssl.org/index.php/Command_Line_Elliptic_Curve_Operations>

## Firmware Update

To perform a firmware update, the `MISO` application needs to fetch an unecrypted firmware image signed in a MCUboot container

These files can be stored in a HTTP server that must support [range requests](https://developer.mozilla.org/en-US/docs/Web/HTTP/Range_requests) for the downloads.
The public key is provided in the configuration file.

The firmware update process takes the firmware container and the locally stored public key to perform the validation.

> FW download has been tested on a non-public (https://nginx.org) server.

### Generate the private key

> This is an example using openssl. Please keep the private key secret and offline.

```console
openssl ecparam -genkey -name prime256v1 -noout -out fw_private_key.pem
```

### Public Key in PEM Format

Generate the public key in PEM format.

```console
openssl ec -in fw_private_key.pem -pubout -out fw.pub
```

### Public Key in DER Format

```console
openssl ec -in fw_private_key.pem -pubout -outform DER -out fw.der
```

### Convert DER to Base64

```console
openssl base64 --in fw.der --out fw.b64
```

Use the base64 output as a one-line string in the config (`http.key`)

### Generate FW container

Use the MCUBoot [`imgtool`](https://docs.mcuboot.com/imgtool.html) script to sign and generate the fw container.

The firmware container header is `0x80` (128) bytes long and the start address is currently `0x7800`

```console
python .\csrc\mcuboot\mcuboot\scripts\imgtool.py sign -v "0.1.2" -F 0x78000 -R 0xff --header-size 0x80 --pad-header -k .\fw_private_key.pem --overwrite-only --public-key-format full -S 0x78000 --align 4 .\zig-out\firmware\app.bin app.bin
```

Verify the content of the firmware container.

```console
python .\csrc\mcuboot\mcuboot\scripts\imgtool.py dumpinfo .\app.bin
```

## Configuration signature

### Sign Configuration

For signing the configuration, please create a private key

```console
openssl ecparam -genkey -name prime256v1 -noout -out private_key.pem
```

### Generate the public key

```console
openssl ec -in private_key.pem -pubout -out config.pub
```

- `private_key.pem` a private key used for signing the configuration.
- `config.pub` a public key used for signing the configuration.
- `config.sig` a signature file.

```console
openssl dgst -sha256 -sign private_key.pem -out config.sig config.txt
```

Verify Configuration

```console
openssl dgst -sha256 -verify config.pub -signature config.sig config.txt
```
