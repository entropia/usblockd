#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "systick.h"
#include "state.h"

#define MAX_DELAY 3

enum state club_state = CLOSED;
uint32_t last_update = 0;

void set_state(enum state state) {
	club_state = state;
	last_update = ticks;
}

enum state get_state(void) {
	if(time_after(ticks, last_update + HZ * MAX_DELAY))
		club_state = CLOSED;

	return club_state;
}
