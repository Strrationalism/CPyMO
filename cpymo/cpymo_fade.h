#ifndef INCLUDE_CPYMO_FADE
#define INCLUDE_CPYMO_FADE

#include "cpymo_color.h"
#include "cpymo_tween.h"
#include <stdbool.h>

typedef struct {
	cpymo_color col;
	cpymo_tween alpha;
	enum {
		cpymo_fade_in,
		cpymo_fade_out,
		cpymo_fade_disabled,
		cpymo_fade_keep
	} state;
} cpymo_fade;

struct cpymo_engine;

static inline void cpymo_fade_reset(cpymo_fade *fade)
{ 
	fade->state = cpymo_fade_disabled;
	fade->col.r = 255;
	fade->col.g = 255;
	fade->col.b = 255;
}

void cpymo_fade_draw(const struct cpymo_engine *);

void cpymo_fade_start_fadeout(struct cpymo_engine *, float time, cpymo_color col);
void cpymo_fade_start_fadein(struct cpymo_engine *, float time);

#endif
