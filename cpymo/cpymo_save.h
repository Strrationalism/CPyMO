#ifndef INCLUDE_CPYMO_SAVE
#define INCLUDE_CPYMO_SAVE

#include "cpymo_error.h"

struct cpymo_engine;

error_t cpymo_save_save(struct cpymo_engine *e, unsigned short save_id);

#endif
