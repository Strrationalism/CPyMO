#ifndef INCLUDE_CPYMO_MSGBOX_UI
#define INCLUDE_CPYMO_MSGBOX_UI

#include "cpymo_error.h"
#include "cpymo_parser.h"

struct cpymo_engine;

error_t cpymo_msgbox_ui_enter(
	struct cpymo_engine *,
	cpymo_parser_stream_span message,
	error_t (*confirm)(struct cpymo_engine *e, void *data),
	void *confirm_data);

#endif
