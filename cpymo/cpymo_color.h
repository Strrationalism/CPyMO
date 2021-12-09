#ifndef INCLUDE_CPYMO_COLOR
#define INCLUDE_CPYMO_COLOR

#include <stdint.h>

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} cpymo_color;

cpymo_color cpymo_color_error();

#endif
