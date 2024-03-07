#include "board.h"
#include "sl_simple_led.h"

sl_simple_led_context_t led_red_context = {
    .port = BOARD_LED_RED_PORT, .pin = BOARD_LED_RED_PIN, .polarity = SL_SIMPLE_LED_POLARITY_ACTIVE_HIGH};

sl_simple_led_context_t led_orange_context = {
    .port = BOARD_LED_ORANGE_PORT, .pin = BOARD_LED_ORANGE_PIN, .polarity = SL_SIMPLE_LED_POLARITY_ACTIVE_HIGH};

sl_simple_led_context_t led_yellow_context = {
    .port = BOARD_LED_YELLOW_PORT, .pin = BOARD_LED_YELLOW_PIN, .polarity = SL_SIMPLE_LED_POLARITY_ACTIVE_HIGH};

sl_led_t led_red    = {.context   = &led_red_context,
                       .init      = sl_simple_led_init,
                       .turn_on   = sl_simple_led_turn_on,
                       .turn_off  = sl_simple_led_turn_off,
                       .toggle    = sl_simple_led_toggle,
                       .get_state = sl_simple_led_get_state};

sl_led_t led_orange = {.context   = &led_orange_context,
                       .init      = sl_simple_led_init,
                       .turn_on   = sl_simple_led_turn_on,
                       .turn_off  = sl_simple_led_turn_off,
                       .toggle    = sl_simple_led_toggle,
                       .get_state = sl_simple_led_get_state};

sl_led_t led_yellow = {.context   = &led_yellow_context,
                       .init      = sl_simple_led_init,
                       .turn_on   = sl_simple_led_turn_on,
                       .turn_off  = sl_simple_led_turn_off,
                       .toggle    = sl_simple_led_toggle,
                       .get_state = sl_simple_led_get_state};
