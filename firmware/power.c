#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "systick.h"
#include "power.h"

// A1 -> power

void power_init(void) {
	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO1);
}

void set_power(bool on) {
	if(on)
		gpio_set(GPIOA, GPIO1);
	else
		gpio_clear(GPIOA, GPIO1);
}
