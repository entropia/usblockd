#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define logm(format, ...) do { fprintf(stderr, format "\n", ##__VA_ARGS__); } while(0)
#define die(format, ...) do { logm(format, ##__VA_ARGS__); exit(EXIT_FAILURE); } while(0)
#define pdie(string, ...) do { perror(string); exit(EXIT_FAILURE); } while(0)
