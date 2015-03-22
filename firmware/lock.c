#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "systick.h"
#include "lock.h"

// B4 -> lock
// B7 -> unlock

void lock_init(void) {
	rcc_periph_clock_enable(RCC_GPIOB);

	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO4);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO7);
}

void lock(void) {
	gpio_set(GPIOB, GPIO4);
	delay_ms(100);
	gpio_clear(GPIOB, GPIO4);
}

void unlock(void) {
	gpio_set(GPIOB, GPIO7);
	delay_ms(100);
	gpio_clear(GPIOB, GPIO7);
}
