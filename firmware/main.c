#include <stdlib.h>
#include <stdint.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "systick.h"
#include "usb.h"
#include "state.h"
#include "lock.h"

// C13 -> status LED
// A8 -> buzzer

void main(void) {
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	init_systick();
	init_usb();

	// configure status LED
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

	// configure buzzer input
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);

	set_state(CLOSED);

	while(1) {
		usb_poll();

		if(get_state() == CLOSED)
			gpio_set(GPIOC, GPIO13);
		else
			gpio_clear(GPIOC, GPIO13);

		if(gpio_get(GPIOA, GPIO8) == 0 && get_state() == CLOSED)
			lock();
	}
}