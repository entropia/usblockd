#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "systick.h"
#include "lock.h"

// A2 -> lock
// A3 -> unlock

void lock_init(void) {
	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO2);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO3);
}

void lock(void) {
	gpio_set(GPIOA, GPIO2);
	delay_ms(100);
	gpio_clear(GPIOA, GPIO2);
}

void unlock(void) {
	gpio_set(GPIOA, GPIO3);
	delay_ms(100);
	gpio_clear(GPIOA, GPIO3);
}
