#ifndef INCLUDE_CPYMO_TEXTBOX
#define INCLUDE_CPYMO_TEXTBOX

#include <cpymo_backend_text.h>

typedef struct {
	float x, y;
	float width;
	float character_size;
	size_t max_lines;
	cpymo_backend_text *lines;
	cpymo_parser_stream_span text;
} cpymo_textbox;

#endif
