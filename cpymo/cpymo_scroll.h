#ifndef INCLUDE_CPYMO_SCROLL
#define INCLUDE_CPYMO_SCROLL

#include <cpymo_backend_image.h>
#include "cpymo_parser.h"
#include "cpymo_tween.h"
#include "cpymo_assetloader.h"

struct cpymo_engine;

typedef struct {
	cpymo_backend_image img;

	float sx, sy, ex, ey, time, all_time;
	int w, h;
} cpymo_scroll;

static inline void cpymo_scroll_init(cpymo_scroll *s)
{ s->img = NULL; }

static inline void cpymo_scroll_free(cpymo_scroll *s)
{ if (s->img) cpymo_backend_image_free(s); }

static inline void cpymo_scroll_reset(cpymo_scroll *s)
{ cpymo_scroll_free(s); cpymo_scroll_init(s); }

void cpymo_scroll_draw(const cpymo_scroll * s);

error_t cpymo_scroll_start(
	struct cpymo_engine *,
	cpymo_parser_stream_span bgname,
	float sx, float sy,
	float ex, float ey,
	float time);

#endif
