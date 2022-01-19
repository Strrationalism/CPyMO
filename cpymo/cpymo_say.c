#include "cpymo_say.h"

void cpymo_say_init(cpymo_say *out, cpymo_assetloader *loader)
{
	out->msgbox = NULL;
	out->namebox = NULL;
	out->msg_cursor = NULL;

	error_t err;
	err = cpymo_load_msgbox_image(out, cpymo_parser_stream_span_pure("message"), loader);
	if (err != CPYMO_ERR_SUCC) out->msgbox = NULL;

	err = cpymo_load_namebox_image(out, cpymo_parser_stream_span_pure("name"), loader);
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

void cpymo_say_free(cpymo_say *say)
{
	if (say->msgbox) cpymo_backend_image_free(say->msgbox);
	if (say->namebox) cpymo_backend_image_free(say->namebox);
	if (say->msg_cursor) cpymo_backend_image_free(say->msg_cursor);
}

void cpymo_say_draw(const cpymo_say *say)
{
}

error_t cpymo_say_load_msgbox_image(cpymo_say *say, cpymo_parser_stream_span name, cpymo_assetloader *l)
{
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
