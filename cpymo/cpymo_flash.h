#ifndef INCLUDE_CPYMO_FLASH
#define INCLUDE_CPYMO_FLASH

#include "cpymo_color.h"
#include <stdbool.h>

struct cpymo_engine;

typedef struct {
	cpymo_color color;
	bool enable;
} cpymo_flash;

static inline void cpymo_flash_reset(cpymo_flash *flash) {
	flash->enable = false;
}

void cpymo_flash_start(struct cpymo_engine *engine, cpymo_color col, float time);
void cpymo_flash_draw(struct cpymo_engine *engine);

#endif