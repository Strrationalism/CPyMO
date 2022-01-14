#ifndef INCLUDE_CPYMO_COLOR
#define INCLUDE_CPYMO_COLOR

#include <stdint.h>

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} cpymo_color;

extern const cpymo_color cpymo_color_error;
extern const cpymo_color cpymo_color_white;
extern const cpymo_color cpymo_color_black;

static inline cpymo_color cpymo_color_inv(cpymo_color col) 
{
	col.r = 255 - col.r;
	col.g = 255 - col.g;
	col.b = 255 - col.b;
	return col;
}

#endif
