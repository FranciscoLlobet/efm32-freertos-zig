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
#include <em_wdog.h>

#include "board.h"

#define BOARD_WDOG DEFAULT_WDOG

void BOARD_Watchdog_Init(void)
{
    WDOG_Init_TypeDef wdog_init = WDOG_INIT_DEFAULT;
    wdog_init.clkSel            = wdogClkSelULFRCO;
    wdog_init.debugRun          = false;          // When in debug mode, the watchdog is disabled.
    wdog_init.perSel            = wdogPeriod_8k;  //(8s)
    wdog_init.lock              = false;
    wdog_init.enable            = false;
    wdog_init.em2Run            = false;
    wdog_init.em3Run            = false;
    wdog_init.em4Block          = false;

    WDOGn_Init(BOARD_WDOG, &wdog_init);
}

void BOARD_Watchdog_Enable(void) { WDOGn_Enable(BOARD_WDOG, true); }

void BOARD_Watchdog_Disable(void) { WDOGn_Enable(BOARD_WDOG, false); }

void BOARD_Watchdog_Feed(void) { WDOGn_Feed(BOARD_WDOG); }
