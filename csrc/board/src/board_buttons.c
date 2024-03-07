/*
 * board_buttons.c
 *
 *  Created on: 8 nov 2022
 *      Author: Francisco
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
