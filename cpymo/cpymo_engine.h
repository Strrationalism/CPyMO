#ifndef INCLUDE_CPYMO_ENGINE
#define INCLUDE_CPYMO_ENGINE

#include <cpymo_backend_image.h>
#include "cpymo_gameconfig.h"
#include "cpymo_error.h"

typedef struct {
	const cpymo_gameconfig *gameconfig;
	cpymo_backend_image image1;

	bool draw;
} cpymo_engine;

error_t cpymo_engine_init(cpymo_engine *engine, cpymo_gameconfig *gameconfig);
void cpymo_engine_update(cpymo_engine *engine, float delta_time_sec, bool *redraw);
void cpymo_engine_draw(cpymo_engine *engine);

#endif

