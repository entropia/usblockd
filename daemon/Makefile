GIT_REV=$(shell git rev-parse HEAD)

all: main.c
	gcc -o usblockd -Wall -Wextra -Werror -DGIT_REV=\"$(GIT_REV)\" -lmosquitto `pkg-config --cflags --libs libusb-1.0` main.c

clean:
	rm -f usblockd
