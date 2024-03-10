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
#include "sl_simple_button.h"

sl_simple_button_context_t button1_context = {
    .state   = SL_SIMPLE_BUTTON_DISABLED,
    .history = 0,
    .port    = BOARD_BUTTON1_PORT,
    .pin     = BOARD_BUTTON1_PIN,
    .mode    = SL_SIMPLE_BUTTON_MODE_INTERRUPT,
};

sl_simple_button_context_t button2_context = {
    .state   = SL_SIMPLE_BUTTON_DISABLED,
    .history = 0,
    .port    = BOARD_BUTTON2_PORT,
    .pin     = BOARD_BUTTON2_PIN,
    .mode    = SL_SIMPLE_BUTTON_MODE_INTERRUPT,
};

sl_button_t button1 = {.context   = &button1_context,
                       .init      = sl_simple_button_init,
                       .poll      = sl_simple_button_poll_step,
                       .enable    = sl_simple_button_enable,
                       .disable   = sl_simple_button_disable,
                       .get_state = sl_simple_button_get_state};

sl_button_t button2 = {.context   = &button2_context,
                       .init      = sl_simple_button_init,
                       .poll      = sl_simple_button_poll_step,
                       .enable    = sl_simple_button_enable,
                       .disable   = sl_simple_button_disable,
                       .get_state = sl_simple_button_get_state};
