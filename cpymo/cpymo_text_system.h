#ifndef INCLUDE_CPYMO_TEXT_SYSTEM
#define INCLUDE_CPYMO_TEXT_SYSTEM

#include "cpymo_textbox.h"

struct cpymo_textbox_list {
	cpymo_textbox textbox;
	struct cpymo_textbox_list *next;
};

typedef struct {
	struct cpymo_textbox_list *textboxes;
	cpymo_textbox *active_textbox;
} cpymo_text_system;

void cpymo_text_system_init(cpymo_text_system *);
void cpymo_text_system_free(cpymo_text_system *);

void cpymo_text_system_draw(const cpymo_text_system *);

error_t cpymo_text_system_new(
	cpymo_textbox **out,
	float x, float y,
	float width, float height,
	float character_size,
	cpymo_color col,
	cpymo_parser_stream_span text);

error_t cpymo_text_system_new_static(
	float x, float y,
	float width, float height,
	float character_size,
	cpymo_color col,
	cpymo_parser_stream_span text);

void cpymo_text_system_clear(cpymo_text_system *);

#endif
