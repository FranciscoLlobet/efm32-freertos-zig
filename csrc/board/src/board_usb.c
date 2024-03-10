/*
 * Copyright (c) 2022-2024 Francisco Llobet-Blandino and the "Miso Project".
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "board.h"

USBX_STRING_DESC(usb_product_string, 'X', 'D', 'K', ' ', 'A', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n');
USBX_STRING_DESC(usb_manufacturer_string, 'm', 'i', 's', 'o');
USBX_STRING_DESC(usb_serial_string, '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0');

USBX_Init_t usbx;

USBX_BUF(usb_rx_buf, 64);
USBX_BUF(usb_tx_buf, 128);

void BOARD_USB_Init(void)
{
    usbx.serialString       = (void *)&usb_serial_string;
    usbx.productString      = (void *)&usb_product_string;
    usbx.manufacturerString = (void *)&usb_manufacturer_string;
    usbx.vendorId           = (uint16_t)0x108C;
    usbx.productId          = (uint16_t)0x017B;
    usbx.maxPower           = (uint8_t)0xFA;
    usbx.powerAttribute     = (uint8_t)0x80;
    usbx.releaseBcd         = (uint16_t)0x01;
    usbx.useFifo            = 0;
    USBX_init(&usbx);
}

void BOARD_USB_Deinit(void) { USBX_disable(); }