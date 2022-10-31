#ifndef INCLUDE_CPYMO_MSGBOX_UI
#define INCLUDE_CPYMO_MSGBOX_UI

#include "cpymo_error.h"
#include "cpymo_parser.h"

struct cpymo_engine;

error_t cpymo_msgbox_ui_enter(
	struct cpymo_engine *,
	cpymo_str message,
	error_t (*confirm)(struct cpymo_engine *e, void *data),
	void *confirm_data);

typedef void (*cpymo_msgbox_ui_on_closing)(
	bool will_call_confirm, void *userdata);

void cpymo_msgbox_ui_set_on_closing(
	struct cpymo_engine *,
	cpymo_msgbox_ui_on_closing on_closing,
	void *userdata);

#endif
