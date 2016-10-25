#include <stdlib.h>
#include <stdint.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "systick.h"
#include "usb.h"
#include "state.h"
#include "lock.h"
#include "power.h"

// C13 -> status LED
// B9 -> buzzer

void main(void) {
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	init_systick();
	init_usb();
	power_init();
	lock_init();

	// configure status LED
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

	// configure buzzer input
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO9);
	gpio_set(GPIOB, GPIO9); // pull up

	set_state(CLOSED);

	uint32_t button_start = 0;
	while(1) {
		usb_poll();

		if(get_state() == CLOSED)
			gpio_set(GPIOC, GPIO13);
		else
			gpio_clear(GPIOC, GPIO13);

		if(gpio_get(GPIOB, GPIO9) == 0) {
			if(button_start == 0)
				button_start = ticks;
		} else
			button_start = 0;

		if(button_start && time_after(ticks, button_start + (HZ/10)) && get_state() == CLOSED)
			lock();

		if(button_start && time_after(ticks, button_start + HZ * 15))
			lock();
	}
}
