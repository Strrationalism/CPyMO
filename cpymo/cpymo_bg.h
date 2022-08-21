#ifndef INCLUDE_CPYMO_BG
#define INCLUDE_CPYMO_BG

#include <cpymo_backend_image.h>
#include <cpymo_backend_masktrans.h>
#include "cpymo_parser.h"
#include "cpymo_tween.h"

struct cpymo_engine;

typedef struct {
	cpymo_backend_image current_bg;
	float current_bg_x, current_bg_y;
	int current_bg_w, current_bg_h;

	bool redraw;

	// Transformation Effect
	cpymo_backend_image transform_next_bg;
	float transform_next_bg_x, transform_next_bg_y;
	int transform_next_bg_w, transform_next_bg_h;
	cpymo_tween transform_progression;
	void (*transform_draw)(const struct cpymo_engine *);

	cpymo_backend_masktrans trans;

	// Quake
	bool follow_chara_quake;

	// Current background name
	char *current_bg_name;
} cpymo_bg;

static inline void cpymo_bg_init(cpymo_bg *bg)
{
	bg->current_bg = NULL;
	bg->current_bg_x = 0;
	bg->current_bg_y = 0;
	bg->transform_next_bg = NULL;
	bg->transform_draw = NULL;
	bg->follow_chara_quake = false;
	bg->trans = NULL;
	bg->current_bg_name = NULL;

	bg->redraw = false;
}

void cpymo_bg_free(cpymo_bg *);

static inline void cpymo_bg_reset(cpymo_bg *bg)
{ cpymo_bg_free(bg); cpymo_bg_init(bg); }

error_t cpymo_bg_update(cpymo_bg *, bool *redraw);

void cpymo_bg_draw(const struct cpymo_engine *);
void cpymo_bg_draw_transform_effect(const struct cpymo_engine *);

error_t cpymo_bg_command(
	struct cpymo_engine *engine,
	cpymo_bg *,
	cpymo_string bgname,
	cpymo_string transition,
	float x,
	float y,
	float time);

static inline void cpymo_bg_follow_chara_quake(cpymo_bg *bg, bool enabled)
{ 
#ifndef LOW_FRAME_RATE
	bg->follow_chara_quake = enabled;
#endif
}

#endif