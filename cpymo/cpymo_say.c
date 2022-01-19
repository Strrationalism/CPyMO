#include "cpymo_say.h"
#include "cpymo_engine.h"

#define DISABLE_TEXTBOX(SAY) \
	if (SAY->textbox_usable) { \
		cpymo_textbox_free(&(SAY)->textbox); \
		SAY->textbox_usable = false; \
	}

#define ENABLE_TEXTBOX(SAY, X, Y, W, H, FONT_SIZE, COL, TEXT, ERR) \
	{ \
		DISABLE_TEXTBOX(SAY); \
		ERR = cpymo_textbox_init( \
			&(SAY)->textbox, X, Y, W, H, FONT_SIZE, COL, TEXT); \
		if (ERR == CPYMO_ERR_SUCC) SAY->textbox_usable = true; \
	}

static void cpymo_say_lazy_init(cpymo_say *out, cpymo_assetloader *loader)
{
	if (out->lazy_init == false) {
		out->lazy_init = true;

		error_t err;
		err = cpymo_say_load_msgbox_image(out, cpymo_parser_stream_span_pure("message"), loader);
		if (err != CPYMO_ERR_SUCC) out->msgbox = NULL;

		err = cpymo_say_load_namebox_image(out, cpymo_parser_stream_span_pure("name"), loader);
		if (err != CPYMO_ERR_SUCC) out->namebox = NULL;

		err = cpymo_assetloader_load_system_image(
			&out->msg_cursor,
			&out->msg_cursor_w,
			&out->msg_cursor_h,
			cpymo_parser_stream_span_pure("message_cursor"),
			"png",
			loader,
			cpymo_gameconfig_is_symbian(loader->game_config));

		if (err != CPYMO_ERR_SUCC) out->msg_cursor = NULL;
	}
}

void cpymo_say_init(cpymo_say *out)
{
	out->msgbox = NULL;
	out->namebox = NULL;
	out->msg_cursor = NULL;
	out->active = false;
	out->lazy_init = false;
	out->textbox_usable = false;
}

void cpymo_say_free(cpymo_say *say)
{
	DISABLE_TEXTBOX(say);
	if (say->msgbox) cpymo_backend_image_free(say->msgbox);
	if (say->namebox) cpymo_backend_image_free(say->namebox);
	if (say->msg_cursor) cpymo_backend_image_free(say->msg_cursor);
}

void cpymo_say_draw(const struct cpymo_engine *e)
{
	if (e->say.active) {
		if (e->say.msgbox) {	// draw message box image
			float x = (float)e->gameconfig.imagesize_w / 2 - (float)e->say.msgbox_w / 2;
			float y = (float)e->gameconfig.imagesize_h - (float)e->say.msgbox_h;

			cpymo_backend_image_draw(
				x, y, (float)e->say.msgbox_w, (float)e->say.msgbox_h,
				e->say.msgbox, 0, 0, (float)e->say.msgbox_w, (float)e->say.msgbox_h,
				1.0f, cpymo_backend_image_draw_type_textbox);
		}

		if (e->say.textbox_usable) {
			cpymo_textbox_draw(&e->say.textbox, cpymo_backend_image_draw_type_text_say);
		}
	}
}

error_t cpymo_say_load_msgbox_image(cpymo_say *say, cpymo_parser_stream_span name, cpymo_assetloader *l)
{
	cpymo_say_lazy_init(say, l);

	if (say->msgbox) cpymo_backend_image_free(say->msgbox);
	say->msgbox = NULL;

	error_t err = cpymo_assetloader_load_system_image(
		&say->msgbox,
		&say->msgbox_w,
		&say->msgbox_h,
		name,
		"png",
		l,
		cpymo_gameconfig_is_symbian(l->game_config));

	if (err != CPYMO_ERR_SUCC) say->msgbox = NULL;

	return err;
}

error_t cpymo_say_load_namebox_image(cpymo_say *say, cpymo_parser_stream_span name, cpymo_assetloader *l)
{
	cpymo_say_lazy_init(say, l);

	if (say->namebox) cpymo_backend_image_free(say->namebox);
	say->namebox = NULL;

	error_t err = cpymo_assetloader_load_system_image(
		&say->namebox,
		&say->namebox_w,
		&say->namebox_h,
		name,
		"png",
		l,
		cpymo_gameconfig_is_symbian(l->game_config));

	if (err != CPYMO_ERR_SUCC) say->namebox = NULL;

	return err;
}

error_t cpymo_say_start(struct cpymo_engine *e, cpymo_parser_stream_span name, cpymo_parser_stream_span text)
{
	cpymo_say_lazy_init(&e->say, &e->assetloader);

	float msglr_l = (float)e->gameconfig.msglr_l * e->gameconfig.imagesize_w / 540.0f;
	float msglr_r = (float)e->gameconfig.msglr_r * e->gameconfig.imagesize_w / 540.0f;
	float msgtb_t = (float)e->gameconfig.msgtb_t * e->gameconfig.imagesize_h / 360.0f;
	float msgtb_b = (float)e->gameconfig.msgtb_b * e->gameconfig.imagesize_h / 360.0f;

	float msgbox_img_x =
		(float)e->gameconfig.imagesize_w / 2 - (float)e->say.msgbox_w / 2;

	float x = msgbox_img_x + msglr_l;
	float w = (float)e->say.msgbox_w - msglr_l - msglr_r;
	float y = 
		(float)e->gameconfig.imagesize_h 
		- (float)e->say.msgbox_h 
		+ msgtb_t;
	float h = (float)e->say.msgbox_h - msgtb_t - msgtb_b;

	float fontsize = cpymo_gameconfig_font_size(&e->gameconfig);

	cpymo_say *say = &e->say;

	error_t err;
	ENABLE_TEXTBOX(say, x, y, w, h, fontsize, e->gameconfig.textcolor, text, err);

	cpymo_textbox_finalize(&say->textbox);

	cpymo_engine_request_redraw(e);

	e->say.active = true;

	return err;
}
