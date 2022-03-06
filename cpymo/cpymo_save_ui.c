#include "cpymo_save_ui.h"
#include "cpymo_engine.h"
#include "cpymo_list_ui.h"
#include "cpymo_save.h"
#include "cpymo_msgbox_ui.h"
#include <assert.h>

#define MAX_SAVES 10

typedef struct {
	cpymo_backend_text text;
	bool is_empty_save;
} cpymo_save_ui_item;

typedef struct {
	cpymo_save_ui_item items[MAX_SAVES];
	bool is_load_ui;
} cpymo_save_ui;

static void cpymo_save_ui_draw_node(const cpymo_engine *e, const void *node_to_draw, float y)
{
	const cpymo_save_ui *ui = (const cpymo_save_ui *)cpymo_list_ui_data_const(e);
	const cpymo_save_ui_item *item = &ui->items[CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(node_to_draw)];

	cpymo_backend_text_draw(
		item->text,
		0,
		y + cpymo_gameconfig_font_size(&e->gameconfig),
		cpymo_color_white,
		1.0f,
		cpymo_backend_image_draw_type_ui_element);
}

static error_t cpymo_save_confirm(cpymo_engine *e, void *data)
{
	uintptr_t save_id = CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(data);
	error_t err = cpymo_save_write(e, (unsigned short)save_id);

	char message[128];
	if (err == CPYMO_ERR_SUCC) {
		sprintf(message, "已经保存到存档 %d", (int)save_id);
	}
	else {
		sprintf(message, "保存失败：%s", cpymo_error_message(err));
	}

	return cpymo_msgbox_ui_enter(
		e,
		cpymo_parser_stream_span_pure(message),
		NULL,
		NULL);
}

static error_t cpymo_save_ui_ok(cpymo_engine *e, void *selected)
{
	cpymo_save_ui *ui = (cpymo_save_ui *)cpymo_list_ui_data(e);

	uintptr_t save_id = CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(selected);

	if (ui->is_load_ui) {
		if (!ui->items[save_id].is_empty_save)
			return cpymo_save_ui_load_savedata_yesnobox(e, (unsigned short)save_id);
		else return CPYMO_ERR_SUCC;
	}
	else {
		char text[48];
		sprintf(text, "确定要保存到存档 %d 吗？", (int)save_id);
		return cpymo_msgbox_ui_enter(
			e,
			cpymo_parser_stream_span_pure(text),
			&cpymo_save_confirm,
			selected);
	}
}

static void cpymo_save_ui_deleter(cpymo_engine *e, void *ui_data)
{
	cpymo_save_ui *ui = (cpymo_save_ui *)cpymo_list_ui_data(e);

	for (size_t i = 0; i < MAX_SAVES; ++i) {
		if (ui->items[i].text) 
			cpymo_backend_text_free(ui->items[i].text);
	}
}

static void* cpymo_save_ui_get_next(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	uintptr_t i = CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(cur);
	if (i >= MAX_SAVES - 1) 
		return NULL;
	else return CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(i + 1);
}

static void* cpymo_save_ui_get_prev(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	const cpymo_save_ui *ui = (const cpymo_save_ui *)cpymo_list_ui_data_const(e);

	uintptr_t i = CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(cur);
	if (i == (ui->is_load_ui ? 0 : 1)) return NULL;
	else return CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(i - 1);
}

error_t cpymo_save_ui_enter(cpymo_engine *e, bool is_load_ui)
{
	cpymo_save_ui *ui = NULL;
	uintptr_t first = is_load_ui ? 0 : 1;
	cpymo_list_ui_enter(
		e,
		(void **)&ui,
		sizeof(*ui),
		&cpymo_save_ui_draw_node,
		&cpymo_save_ui_ok,
		&cpymo_save_ui_deleter,
		CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(first),
		&cpymo_save_ui_get_next,
		&cpymo_save_ui_get_prev,
		false,
		4);

	ui->is_load_ui = is_load_ui;

	for (size_t i = 0; i < MAX_SAVES; ++i) {
		ui->items[i].text = NULL;
		ui->items[i].is_empty_save = false;
	}

	const float fontsize = cpymo_gameconfig_font_size(&e->gameconfig);
	size_t characters = (size_t)((float)e->gameconfig.imagesize_w / fontsize * 1.3f);

	char text_buf[1024];
	for (size_t i = is_load_ui ? 0 : 1; i < MAX_SAVES; ++i) {
		FILE *save = cpymo_save_open_read(e, (unsigned short)i);
		cpymo_save_title title;
		title.say_name = NULL;
		title.say_text = NULL;
		title.title = NULL;

		if (save) {
			error_t err = cpymo_save_load_title(&title, save);
			fclose(save);
			if (err != CPYMO_ERR_SUCC) ui->items[i].is_empty_save = true;
		}
		else {
			ui->items[i].is_empty_save = true;
		}

		if (!ui->items[i].is_empty_save) {
			assert(title.say_name != NULL);
			assert(title.title != NULL);
			assert(title.say_text != NULL);

			char *tmp_str = (char *)malloc(strlen(title.say_name) + strlen(title.say_text) + 16);
			if (tmp_str == NULL) {
				cpymo_ui_exit(e);
				free(title.say_name);
				free(title.say_text);
				free(title.title);
				return CPYMO_ERR_OUT_OF_MEM;
			}

			*tmp_str = '\0';
			if (strlen(title.say_name) > 0) {
				strcat(tmp_str, title.say_name);
				strcat(tmp_str, " ");
			}

			strcat(tmp_str, title.say_text);

			cpymo_parser_stream_span say_preview_text_tail = cpymo_parser_stream_span_pure(tmp_str);
			cpymo_parser_stream_span say_preview_text = 
				cpymo_parser_stream_span_split(&say_preview_text_tail, characters);

			if (say_preview_text_tail.len > 0) {
				*(char *)say_preview_text_tail.begin = '\0';

				if (say_preview_text.len > 3) {
					((char *)say_preview_text.begin)[say_preview_text.len - 1] = '.';
					((char *)say_preview_text.begin)[say_preview_text.len - 2] = '.';
					((char *)say_preview_text.begin)[say_preview_text.len - 3] = '.';
				}
			}

			if (i == 0) sprintf(text_buf, "自动存档  %s\n%s", title.title, tmp_str);
			else sprintf(text_buf, "存档 %d      %s\n%s", (int)i, title.title, tmp_str);

			free(tmp_str);

			free(title.say_name);
			free(title.title);
			free(title.say_text);
		}
		else {
			if (i == 0) sprintf(text_buf, "自动存档  空");
			else sprintf(text_buf, "存档 %d      空", (int)i);
		}

		float w;
		error_t err = cpymo_backend_text_create(
			&ui->items[i].text, &w, cpymo_parser_stream_span_pure(text_buf), fontsize);

		if (err != CPYMO_ERR_SUCC) {
			cpymo_ui_exit(e);
			return err;
		}
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_save_ui_load_savedata_yesnobox_confirm(cpymo_engine *e, void *save_id_x)
{
	return CPYMO_ERR_SUCC;
}

error_t cpymo_save_ui_load_savedata_yesnobox(cpymo_engine * e, unsigned short save_id)
{
	char str[48];
	if (save_id) {
		sprintf(str, "确定要加载存档 %d 吗？", save_id);
	}
	else {
		strcpy(str, "确定要加载自动存档吗？");
	}

	return cpymo_msgbox_ui_enter(
		e,
		cpymo_parser_stream_span_pure(str),
		&cpymo_save_ui_load_savedata_yesnobox_confirm,
		(void *)(uintptr_t)save_id);
}

