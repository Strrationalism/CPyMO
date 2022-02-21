#ifndef INCLUDE_CPYMO_MUSIC_BOX
#define INCLUDE_CPYMO_MUSIC_BOX

#include "cpymo_list_ui.h"
#include "cpymo_parser.h"
#include <cpymo_backend_text.h>

typedef struct {
	char *music_list;

	uintptr_t music_count;
	cpymo_parser_stream_span *music_filename;
	cpymo_backend_text *music_title;
	float font_size;
} cpymo_music_box;

error_t cpymo_music_box_enter(struct cpymo_engine *);

#endif
