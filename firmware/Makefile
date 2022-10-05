TOPDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJ = main.o systick.o usb.o state.o lock.o power.o

CROSS_PREFIX ?= arm-none-eabi-
CC=$(CROSS_PREFIX)gcc
OBJCOPY=$(CROSS_PREFIX)objcopy
SIZE=$(CROSS_PREFIX)size

CFLAGS = -Wall -Werror -Wextra -std=gnu99 -I../libopencm3/include -I ../common -DSTM32F1 -mcpu=cortex-m3 -mthumb -fno-common -ffreestanding -Og -g
LDFLAGS = -lopencm3_stm32f1 -L../libopencm3/lib -mthumb -mcpu=cortex-m3 -nostartfiles -Wl,--gc-sections -flto -T stm32f100x8.ld -g

all: firmware.bin

firmware.elf: $(OBJ)
	$(CC) -o firmware.elf $^ $(LDFLAGS)
	$(SIZE) -B firmware.elf

%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

flash: firmware.bin
	echo "reset halt; flash write_image erase unlock $(TOPDIR)/firmware.bin 0x8000000; reset" | nc 127.0.0.1 4444

clean:
	rm -f *.o firmware.elf firmware.bin

debug:
	arm-none-eabi-gdb  -ex 'target extended-remote :3333' firmware.elf

.PHONY: all flash clean debug
