#ifndef INCLUDE_CPYMO_SAVE
#define INCLUDE_CPYMO_SAVE

#include <stdio.h>
#include "cpymo_error.h"

struct cpymo_engine;

error_t cpymo_save_write(struct cpymo_engine *e, unsigned short save_id);
void cpymo_save_autosave(struct cpymo_engine *e);

FILE * cpymo_save_open_read(struct cpymo_engine *e, unsigned short save_id);

typedef struct {
	char *title;
	char *say_name, *say_text;
} cpymo_save_title;

error_t cpymo_save_load_title(cpymo_save_title *out, FILE *save);

error_t cpymo_save_load_savedata(struct cpymo_engine *e, FILE *save);

#endif
