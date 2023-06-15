/*
 * board_buttons.c
 *
 *  Created on: 8 nov 2022
 *      Author: Francisco
 */

#include "board.h"

#include "sl_simple_button.h"

sl_simple_button_context_t button1_context =
{ .state = SL_SIMPLE_BUTTON_DISABLED, .history = 0, .port = BOARD_BUTTON1_PORT,
		.pin = BOARD_BUTTON1_PIN, .mode = SL_SIMPLE_BUTTON_MODE_INTERRUPT, };

sl_simple_button_context_t button2_context =
{ .state = SL_SIMPLE_BUTTON_DISABLED, .history = 0, .port = BOARD_BUTTON2_PORT,
		.pin = BOARD_BUTTON2_PIN, .mode = SL_SIMPLE_BUTTON_MODE_INTERRUPT, };

sl_button_t button1 =
{ .context = &button1_context, .init = sl_simple_button_init, .poll =
		sl_simple_button_poll_step, .enable = sl_simple_button_enable,
		.disable = sl_simple_button_disable, .get_state =
				sl_simple_button_get_state };

sl_button_t button2 =
{ .context = &button2_context, .init = sl_simple_button_init, .poll =
		sl_simple_button_poll_step, .enable = sl_simple_button_enable,
		.disable = sl_simple_button_disable, .get_state =
				sl_simple_button_get_state };

void sl_button_on_change(const sl_button_t *handle)
{
	if (handle == &button1)
	{
		if (SL_SIMPLE_BUTTON_PRESSED
				== SL_SIMPLE_BUTTON_GET_STATE(&button1_context))
		{
			// TODO: Send message to event manager (?)
			sl_led_turn_on(&led_red);
		}
		else
		{
			// TODO: Send message to event manager (?)
			sl_led_turn_off(&led_red);
		}

	}
	else if (handle == &button2)
	{
		if (SL_SIMPLE_BUTTON_PRESSED
				== SL_SIMPLE_BUTTON_GET_STATE(&button2_context))
		{
			// TODO: Send message to event manager (?)
			sl_led_turn_on(&led_orange);
		}
		else
		{
			// TODO: Send message to event manager (?)
			sl_led_turn_off(&led_orange);
		}
	}
	else
	{
		// TODO: Send ERROR message to event manager (?)
	}

}
