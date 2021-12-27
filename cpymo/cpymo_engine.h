#ifndef INCLUDE_CPYMO_ENGINE
#define INCLUDE_CPYMO_ENGINE

#include "cpymo_assetloader.h"
#include "cpymo_gameconfig.h"
#include "cpymo_error.h"

typedef struct {
	cpymo_gameconfig gameconfig;
	cpymo_assetloader assetloader;
	bool draw;
} cpymo_engine;

error_t cpymo_engine_init(cpymo_engine *out, const char *gamedir);
void cpymo_engine_free(cpymo_engine *engine);
void cpymo_engine_update(cpymo_engine *engine, float delta_time_sec, bool *redraw);
void cpymo_engine_draw(cpymo_engine *engine);

#endif

