# Miso License and Licensing

## Licensing Strategy

`miso` code is licensed using two open source licenses depending on the programming language used in the file.

- [MIT License](#mit-license) is used in `zig`-code and `python` code
- [BSD 3-Clause License](#bsd-3-clause-license) is used in `C`-code and headers

For contributions, please consult [CONTRIBUTING.md](./CONTRIBUTING.md)

## SBOM and 3rd Party Software

`miso` uses 3rd party software extensively. Some of the included submodules may include licenses not listed below. If a 3rd party component offer dual licensing, please use the most convenient or compatible with the MIT license or the BSD 3-Clause licenses used in the project.

| Product                 | Licensing info  |Version    | Year | SPDX Identifier             |
|-------------------------|---------------------------------------------------------------------|-----------|------|-----------------------------|
| FreeRTOS Kernel   | [LICENSE.md](./csrc/system/FreeRTOS-Kernel/LICENSE.md)              | V11.0.x | 2024 | MIT                         |
| Wakaama                 | [NOTICE.md](./csrc/connectivity/wakaama/NOTICE.md)                  | dev branch        | 2024 | EPL-2.0, EDL-1.0            |
| PicoHTTP Parser         | [README.md](./csrc/connectivity/picohttpparser/README.md)           | master branch        | 2024 | MIT License, GPL-2.0-or-later |
| Eclipse Paho MQTT C/C++ | [notice.html](./csrc/connectivity/paho.mqtt.embedded-c/notice.html) | 1.3.9      | 2021 | EPL-1.0, EDL-1.0            |
| mbedTLS                 | [LICENSE](./csrc/crypto/mbedtls/LICENSE)                            | 3.5.2      | 2024 | Apache-2.0                  |
| McuBoot                 | [LICENSE](./csrc/mcuboot/LICENSE)                                   | 2.1.0-dev      | 2024 | Apache-2.0                  |
| FatFs                   | [ff.h](./csrc/system/ff15/source/ff.h)                              | R0.15      | 2023 | Custom (similar to MIT/BSD) |
| CC3100 SDK              | [readme.txt](./csrc/system/cc3100-sdk/readme.txt)                   | 1.3.0      | 2015 | BSD-Style                   |
| Gecko SDK               | [License.txt](./csrc/system/gecko_sdk/License.txt)                  | 4.4.1      | 2024 | Zlib                        |
| picolibc                | |1.8.6      | 2023 | MIT License                 |
| Zig                     | |0.11.0     | 2023 | MIT License                 |
| Jsmn                    | [LICENSE](./csrc/utils/jsmn/LICENSE) | master branch | 2021 | MIT License                 |

## MIT License

Copyright (c) 2023-2024 Francisco Llobet-Blandino and the "Miso Project".

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## BSD 3-Clause License

Copyright (c) 2022-2024 Francisco Llobet-Blandino and the "Miso Project".

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
