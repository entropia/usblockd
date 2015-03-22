#pragma once

enum state {
	OPEN, CLOSED
};

void set_state(enum state state);
enum state get_state(void);
