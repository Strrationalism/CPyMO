#ifndef INCLUDE_CPYMO_TEXT
#define INCLUDE_CPYMO_TEXT

#include "cpymo_textbox.h"

struct cpymo_engine;

struct cpymo_textbox_list {
	struct cpymo_textbox_list *next;
	cpymo_textbox box;
};

typedef struct {
	struct cpymo_textbox_list *ls;
	cpymo_textbox *active_box;
} cpymo_text;

void cpymo_text_init(cpymo_text *);
void cpymo_text_free(cpymo_text *);

void cpymo_text_draw(const struct cpymo_engine *);

error_t cpymo_text_new(
	struct cpymo_engine *,
	float x1, float y1, float x2, float y2,
	cpymo_color col, float fontsize,
	cpymo_str text,
	bool immediately);

void cpymo_text_clear(cpymo_text *);

#endif
