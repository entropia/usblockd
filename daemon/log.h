#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define error(format, ...) do { fprintf(stderr, format "\n", ##__VA_ARGS__); } while(0)
#define die(format, ...) do { error(format, ##__VA_ARGS__); exit(EXIT_FAILURE); } while(0)
#define pdie(string, ...) do { perror(string); exit(EXIT_FAILURE); } while(0)
