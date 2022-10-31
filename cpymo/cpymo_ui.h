#ifndef INCLUDE_CPYMO_UI
#define INCLUDE_CPYMO_UI

#include "cpymo_error.h"
#include <stddef.h>
#include <stdbool.h>

struct cpymo_engine;

typedef error_t(*cpymo_ui_updater)(struct cpymo_engine *, void *ui_data, float);

typedef void(*cpymo_ui_drawer)(const struct cpymo_engine *, const void *ui_data);

typedef void(*cpymo_ui_deleter)(struct cpymo_engine *, void *ui_data);

struct cpymo_ui;

error_t cpymo_ui_enter(
	void **out_uidata, 
	struct cpymo_engine *,
	size_t ui_data_size,
	cpymo_ui_updater,
	cpymo_ui_drawer,
	cpymo_ui_deleter);

void cpymo_ui_exit(struct cpymo_engine *);

void *cpymo_ui_data(struct cpymo_engine *);
const void *cpymo_ui_data_const(const struct cpymo_engine *);

error_t cpymo_ui_update(struct cpymo_engine *, float dt);
void cpymo_ui_draw(const struct cpymo_engine *);

bool cpymo_ui_enabled(const struct cpymo_engine *);

#endif
