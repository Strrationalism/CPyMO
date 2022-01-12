#ifndef INCLUDE_CPYMO_FLOATING_HINT
#define INCLUDE_CPYMO_FLOATING_HINT

#include <cpymo_backend_image.h>
#include <cpymo_backend_text.h>
#include "cpymo_parser.h"

struct cpymo_engine;

typedef struct {
	cpymo_backend_text text;
	cpymo_backend_image background;
	int background_w, background_h;

	cpymo_color color;

	float x, y;
	float time;
} cpymo_floating_hint;

static inline void cpymo_floating_hint_init(cpymo_floating_hint *h)
{
	h->text = NULL;
	h->background = NULL;
	h->x = 0;
	h->y = 0;
	h->time = 0;
	h->background_w = 0;
	h->background_h = 0;
}

void cpymo_floating_hint_free(cpymo_floating_hint *);

void cpymo_floating_hint_draw(const cpymo_floating_hint *);

error_t cpymo_floating_hint_start(
	struct cpymo_engine *,
	cpymo_parser_stream_span text,
	cpymo_parser_stream_span background,
	float x,
	float y,
	cpymo_color col,
	float fontscale);

#endif
