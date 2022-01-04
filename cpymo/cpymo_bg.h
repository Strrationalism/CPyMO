#ifndef INCLUDE_CPYMO_BG
#define INCLUDE_CPYMO_BG

#include <cpymo_backend_image.h>
#include "cpymo_parser.h"

struct cpymo_engine;

/*** Simple Impl ***/

typedef struct {
	cpymo_backend_image current_bg;
	float current_bg_x, current_bg_y;
	int current_bg_w, current_bg_h;

	bool redraw;
} cpymo_bg;

static void cpymo_bg_init(cpymo_bg *bg)
{
	bg->current_bg = NULL; 
	bg->current_bg_x = 0;
	bg->current_bg_y = 0;

	bg->redraw = false;
}

void cpymo_bg_free(cpymo_bg *);

error_t cpymo_bg_update(cpymo_bg *, bool *redraw);
void cpymo_bg_draw(cpymo_bg *);

error_t cpymo_bg_command(
	struct cpymo_engine *engine,
	cpymo_bg *,
	cpymo_parser_stream_span bgname,
	cpymo_parser_stream_span transition,
	float x,
	float y);

#endif