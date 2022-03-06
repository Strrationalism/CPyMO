#ifndef INCLUDE_CPYMO_SAVE_UI
#define INCLUDE_CPYMO_SAVE_UI

#include "cpymo_error.h"
#include <stdbool.h>

struct cpymo_engine;

error_t cpymo_save_ui_enter(struct cpymo_engine *, bool is_load_ui);

error_t cpymo_save_ui_load_savedata_yesnobox(struct cpymo_engine *e, unsigned short save_id);

#endif
