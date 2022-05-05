#ifndef INCLUDE_CPYMO_SAVE_GLOBAL
#define INCLUDE_CPYMO_SAVE_GLOBAL

#include "cpymo_error.h"

struct cpymo_engine;

error_t cpymo_save_global_load(struct cpymo_engine *);
error_t cpymo_save_global_save(struct cpymo_engine *);

error_t cpymo_save_config_save(const struct cpymo_engine *);
error_t cpymo_save_config_load(struct cpymo_engine *);

#endif
