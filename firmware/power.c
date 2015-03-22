#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "systick.h"
#include "power.h"

// XX -> power

void power_init(void) {
	rcc_periph_clock_enable(RCC_GPIOB);

	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO4);
}

void set_power(bool on) {
	if(on)
		gpio_set(GPIOB, GPIO4);
	else
		gpio_clear(GPIOB, GPIO4);
}
