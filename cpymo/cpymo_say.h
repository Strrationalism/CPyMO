#ifndef INCLUDE_CPYMO_SAY
#define INCLUDE_CPYMO_SAY

#include "cpymo_textbox.h"
#include "cpymo_assetloader.h"
#include "cpymo_key_hold.h"

struct cpymo_engine;

typedef struct {
	cpymo_backend_image msgbox, namebox, msg_cursor;
	int msgbox_w, msgbox_h, namebox_w, namebox_h, msg_cursor_w, msg_cursor_h;

	cpymo_textbox textbox;
	bool textbox_usable;

	bool active;
	bool lazy_init;

	bool hide_window;

	cpymo_backend_text name;
	float name_width;

	cpymo_key_hold key_mouse_button;

	char *msgbox_name, *namebox_name;
} cpymo_say;

void cpymo_say_init(cpymo_say *);
void cpymo_say_free(cpymo_say *);

void cpymo_say_draw(const struct cpymo_engine *);

error_t cpymo_say_load_msgbox_and_namebox_image(
	cpymo_say *, cpymo_parser_stream_span msgbox, cpymo_parser_stream_span namebox, cpymo_assetloader *);

error_t cpymo_say_start(
	struct cpymo_engine *,
	cpymo_parser_stream_span name, 
	cpymo_parser_stream_span text);

void cpymo_say_hidewindow_until_click(struct cpymo_engine *e);

#endif
