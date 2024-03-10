# Miso License and Licensing

## Licensing Strategy

`miso` code is licensed using two open source licenses depending on the programming language used in the file.

- [MIT License](#mit-license) is used in `zig`-code
- [BSD 3-Clause License](#bsd-3-clause-license) is used in `C`-code and headers

For contributions, please consult [CONTRIBUTING.md](./CONTRIBUTING.md)

## SBOM and 3rd Party Software

`miso` uses 3rd party software extensively. Some of the included submodules may include licenses not listed below or be dual-licensed. If a 3rd party component offer dual licensing, please use the most convenient or compatible with the MIT license or the BSD 3-Clause licenses used in the project.

| Product                 | Version    | Year | SPDX Identifier             |
|-------------------------|------------|------|-----------------------------|
| FreeRTOS Kernel         | 202112.00  | 2021 | MIT                         |
| Wakaama                 | 1.1        | 2021 | EPL-2.0, EDL-1.0            |
| PicoHTTP Parser         | 2.9        | 2020 | MIT License, GPL-2.0-or-later |
| Eclipse Paho MQTT C/C++ | 1.3.9      | 2021 | EPL-1.0, EDL-1.0            |
| mbedTLS                 | 3.1.0      | 2021 | Apache-2.0                  |
| McuBoot                 | 1.8.0      | 2021 | Apache-2.0                  |
| FatFs                   | R0.14c     | 2020 | Custom (similar to MIT/BSD) |
| CC3100 SDK              | 1.3.0      | 2015 | BSD-Style                   |
| Gecko SDK               | 3.2.1      | 2021 | Zlib                        |
| picolibc                | 1.7.4      | 2021 | MIT License                 |
| Zig                     | 0.9.0      | 2021 | MIT License                 |
| Jsmn                    | 2020.10.19 | 2020 | MIT License                 |

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
