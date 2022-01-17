#ifndef INCLUDE_CPYMO_TEXTBOX
#define INCLUDE_CPYMO_TEXTBOX

#include <cpymo_backend_text.h>

typedef struct {
	float x, y;
	float width;
	float character_size;
	size_t max_lines;
	cpymo_backend_text *lines;
	size_t active_line;
	cpymo_parser_stream_span text_curline_and_remaining;
	cpymo_color color;
} cpymo_textbox;

error_t cpymo_textbox_init(
	cpymo_textbox **out,
	float x, float y,
	float width, float height,
	float character_size,
	cpymo_color col);

void cpymo_textbox_free(
	cpymo_textbox *);

void cpymo_textbox_draw(
	cpymo_textbox *);

void cpymo_textbox_show_next_line(cpymo_textbox *);

void cpymo_textbox_show_next_char(cpymo_textbox *);

bool cpymo_textbox_page_full(cpymo_textbox *);

void cpymo_textbox_show_next_page(cpymo_textbox *);

void cpymo_textbox_clear_page(cpymo_textbox *);

bool cpymo_textbox_all_finished(cpymo_textbox *);

void cpymo_textbox_finalize(cpymo_textbox *);

#endif
