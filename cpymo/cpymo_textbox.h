#ifndef INCLUDE_CPYMO_TEXTBOX
#define INCLUDE_CPYMO_TEXTBOX

#include <cpymo_backend_text.h>

struct cpymo_engine;

typedef struct {
	float x, y;
	float width;
	float character_size;
	size_t max_lines;
	cpymo_backend_text *lines;
	size_t active_line;
	float active_line_current_width;
	cpymo_parser_stream_span text_curline_and_remaining;
	size_t text_curline_size;
	cpymo_color color;

	float timer;
} cpymo_textbox;

error_t cpymo_textbox_init(
	cpymo_textbox *out,
	float x, float y,
	float width, float height,
	float character_size,
	cpymo_color col,
	cpymo_parser_stream_span text);

void cpymo_textbox_free(
	cpymo_textbox *);

void cpymo_textbox_draw(
	const cpymo_textbox *, enum cpymo_backend_image_draw_type drawtype);

static inline bool cpymo_textbox_page_full(cpymo_textbox *tb)
{ return tb->active_line >= tb->max_lines; }

void cpymo_textbox_refresh_curline(cpymo_textbox *);

void cpymo_textbox_show_next_char(cpymo_textbox *);

void cpymo_textbox_clear_page(cpymo_textbox *);

static inline bool cpymo_textbox_all_finished(cpymo_textbox *tb)
{ return tb->text_curline_and_remaining.len == tb->text_curline_size; }

void cpymo_textbox_finalize(cpymo_textbox *);

bool cpymo_textbox_wait_text_fadein(struct cpymo_engine *, float, cpymo_textbox *which_textbox);
bool cpymo_textbox_wait_text_reading(struct cpymo_engine *, float, cpymo_textbox *which_textbox);

#endif
