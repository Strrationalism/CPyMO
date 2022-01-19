#ifndef INCLUDE_CPYMO_ENGINE
#define INCLUDE_CPYMO_ENGINE

#include <cpymo_backend_input.h>
#include "cpymo_assetloader.h"
#include "cpymo_gameconfig.h"
#include "cpymo_error.h"
#include "cpymo_interpreter.h"
#include "cpymo_vars.h"
#include "cpymo_wait.h"
#include "cpymo_flash.h"
#include "cpymo_fade.h"
#include "cpymo_bg.h"
#include "cpymo_anime.h"
#include "cpymo_select_img.h"
#include "cpymo_charas.h"
#include "cpymo_scroll.h"
#include "cpymo_floating_hint.h"
#include "cpymo_say.h"
#include "cpymo_text.h"

struct cpymo_engine {
	cpymo_gameconfig gameconfig;
	cpymo_assetloader assetloader;
	cpymo_vars vars;
	cpymo_interpreter *interpreter;
	cpymo_input prev_input, input;
	cpymo_wait wait;
	cpymo_flash flash;
	cpymo_fade fade;
	cpymo_bg bg;
	cpymo_anime anime;
	cpymo_select_img select_img;
	cpymo_charas charas;
	cpymo_scroll scroll;
	cpymo_floating_hint floating_hint;
	cpymo_say say;
	cpymo_text text;

	bool skipping;
	char *title;

	bool redraw;
};

typedef struct cpymo_engine cpymo_engine;

error_t cpymo_engine_init(cpymo_engine *out, const char *gamedir);
void cpymo_engine_free(cpymo_engine *engine);
error_t cpymo_engine_update(cpymo_engine *engine, float delta_time_sec, bool *redraw);
void cpymo_engine_draw(const cpymo_engine *engine);

static inline bool cpymo_engine_skipping(cpymo_engine *engine)
{ return engine->input.skip || engine->skipping; }

static inline void cpymo_engine_request_redraw(cpymo_engine *engine)
{ engine->redraw = true; }

#define CPYMO_INPUT_JUST_PRESSED(PENGINE, KEY) \
	(!PENGINE->prev_input.KEY && PENGINE->input.KEY)

static inline bool cpymo_input_mouse_moved(cpymo_engine *engine)
{
	if (engine->input.mouse_position_useable && engine->prev_input.mouse_position_useable) {
		return engine->input.mouse_x != engine->prev_input.mouse_x
			|| engine->input.mouse_y != engine->prev_input.mouse_y;
	}
	else {
		return engine->prev_input.mouse_position_useable != engine->input.mouse_position_useable;
	}
}

static inline bool cpymo_input_foward_key_just_pressed(cpymo_engine *e)
{
	return
		CPYMO_INPUT_JUST_PRESSED(e, ok) ||
		CPYMO_INPUT_JUST_PRESSED(e, mouse_button) ||
		CPYMO_INPUT_JUST_PRESSED(e, down) ||
		CPYMO_INPUT_JUST_PRESSED(e, cancel) ||
		CPYMO_INPUT_JUST_PRESSED(e, up) ||
		cpymo_engine_skipping(e);
}

#endif

