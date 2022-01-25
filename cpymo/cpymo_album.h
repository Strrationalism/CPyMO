#ifndef INCLUDE_CPYMO_ALBUM
#define INCLUDE_CPYMO_ALBUM

#include "cpymo_error.h"
#include "cpymo_parser.h"

struct cpymo_engine;

error_t cpymo_album_enter(
	struct cpymo_engine *e, 
	cpymo_parser_stream_span album_list_name, 
	cpymo_parser_stream_span album_ui_name,
	size_t page);

#endif
