#include "cpymo_prelude.h"
#include "cpymo_save_ui.h"
#include "cpymo_engine.h"
#include "cpymo_list_ui.h"
#include "cpymo_save.h"
#include "cpymo_msgbox_ui.h"
#include "cpymo_localization.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define MAX_SAVES 31

typedef struct {
	cpymo_backend_text text;
#ifdef ENABLE_TEXT_EXTRACT
	char *orginal_text;
#endif
	bool is_empty_save;
} cpymo_save_ui_item;

typedef struct {
	cpymo_save_ui_item items[MAX_SAVES];
	bool is_load_ui;
} cpymo_save_ui;

#ifdef DISABLE_AUTOSAVE
#define SAVE_UI_FIRST_SLOT 0
#else
#define SAVE_UI_FIRST_SLOT 1
#endif

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

static error_t cpymo_save_confirm(cpymo_engine *e, void *data, bool confirm)
{
	if (!confirm) return CPYMO_ERR_SUCC;
	uintptr_t save_id = CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(data);
	error_t err = cpymo_save_write(e, (unsigned short)save_id);

	const cpymo_localization *l = cpymo_localization_get(e);

	char *msg = NULL;
	error_t errstr;
	if (err == CPYMO_ERR_SUCC) 
		errstr = l->save_already_save_to(&msg, (int)save_id);
	else 
		errstr = l->save_failed(&msg, err);
	
	if (errstr != CPYMO_ERR_SUCC) return errstr;

	cpymo_ui_exit(e);

	err = cpymo_msgbox_ui_enter(
		e,
		cpymo_str_pure(msg),
		NULL,
		NULL);

	free(msg);

	return err;
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
		char *msg = NULL;
		const cpymo_localization *l = cpymo_localization_get(e);
		error_t err = l->save_are_you_sure_save_to(&msg, (int)save_id);
		CPYMO_THROW(err);
		err = cpymo_msgbox_ui_enter(
			e,
			cpymo_str_pure(msg),
			&cpymo_save_confirm,
			selected);
		free(msg);
		return err;
	}
}

static void cpymo_save_ui_deleter(cpymo_engine *e, void *ui_data)
{
	cpymo_save_ui *ui = (cpymo_save_ui *)cpymo_list_ui_data(e);

	for (size_t i = 0; i < MAX_SAVES; ++i) {
		if (ui->items[i].text) 
			cpymo_backend_text_free(ui->items[i].text);
#ifdef ENABLE_TEXT_EXTRACT
		if (ui->items[i].orginal_text) free(ui->items[i].orginal_text);
#endif
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
	if (i == (ui->is_load_ui ? 0 : SAVE_UI_FIRST_SLOT)) return NULL;
	else return CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(i - 1);
}

#ifdef ENABLE_TEXT_EXTRACT
static error_t cpymo_save_ui_visual_impaired_selection_changed(cpymo_engine *e, void *cur)
{
	if (cur) {
		uintptr_t i = CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(cur);
		const cpymo_save_ui *ui = (const cpymo_save_ui *)cpymo_list_ui_data(e);
		cpymo_backend_text_extract(ui->items[i].orginal_text);
	}
	return CPYMO_ERR_SUCC;
}
#endif

error_t cpymo_save_ui_enter(cpymo_engine *e, bool is_load_ui)
{
	cpymo_save_ui *ui = NULL;
	uintptr_t first = is_load_ui ? 0 : SAVE_UI_FIRST_SLOT;
	error_t err = cpymo_list_ui_enter(
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
		3);
	CPYMO_THROW(err);
	
	cpymo_list_ui_enable_loop(e);

#ifdef ENABLE_TEXT_EXTRACT
	cpymo_list_ui_set_selection_changed_callback(e, &cpymo_save_ui_visual_impaired_selection_changed);
	ui->items[0].orginal_text = NULL;
#endif

	ui->is_load_ui = is_load_ui;

	for (size_t i = 0; i < MAX_SAVES; ++i) {
		ui->items[i].text = NULL;
		ui->items[i].is_empty_save = false;
	}

	const float fontsize = cpymo_gameconfig_font_size(&e->gameconfig);
	size_t characters = (size_t)((float)e->gameconfig.imagesize_w / fontsize * 1.3f);

	char text_buf[1024];
	for (size_t i = is_load_ui ? 0 : SAVE_UI_FIRST_SLOT; i < MAX_SAVES; ++i) {
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

			cpymo_str say_preview_text_tail = cpymo_str_pure(tmp_str);
			cpymo_str say_preview_text = 
				cpymo_str_split(&say_preview_text_tail, characters);

			if (say_preview_text_tail.len > 0) {
				*(char *)say_preview_text_tail.begin = '\0';

				if (say_preview_text.len > 3) {
					((char *)say_preview_text.begin)[say_preview_text.len - 1] = '.';
					((char *)say_preview_text.begin)[say_preview_text.len - 2] = '.';
					((char *)say_preview_text.begin)[say_preview_text.len - 3] = '.';
				}
			}

			const cpymo_localization *l = cpymo_localization_get(e);

			char *str = NULL;
			error_t err;

			#ifndef DISABLE_AUTOSAVE
			if (i == 0) err = l->save_auto_title(&str, title.title);
			else 
			#endif
				err = l->save_title(&str, (int)i, title.title);

			if (err != CPYMO_ERR_SUCC) {
				free(tmp_str);
				free(title.say_name);
				free(title.title);
				free(title.say_text);
				return err;
			}

			snprintf(text_buf, sizeof(text_buf), "%s\n%s", str, tmp_str);
			free(str);

			free(tmp_str);

			free(title.say_name);
			free(title.title);
			free(title.say_text);

			cpymo_utils_replace_str_newline_n(text_buf);
		}
		else {
			error_t err;
			const cpymo_localization *l = cpymo_localization_get(e);
			char *msg = NULL;

			#ifndef DISABLE_AUTOSAVE
			if (i == 0) err = l->save_auto_title(&msg, "");
			else 
			#endif
				err = l->save_title(&msg, (int)i, "");

			CPYMO_THROW(err);
			strncpy(text_buf, msg, sizeof(text_buf) - 1);
			free(msg);
		}

		float w;
		error_t err = cpymo_backend_text_create(
			&ui->items[i].text, &w, cpymo_str_pure(text_buf), fontsize);

#ifdef ENABLE_TEXT_EXTRACT
		ui->items[i].orginal_text = 
			cpymo_str_copy_malloc(cpymo_str_pure(text_buf));
#endif

		if (err != CPYMO_ERR_SUCC) {
			cpymo_ui_exit(e);
			return err;
		}
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_save_ui_load_savedata_yesnobox_confirm(
	cpymo_engine *e, void *save_id_x, bool confirm)
{
	if (!confirm) return CPYMO_ERR_SUCC;
	FILE *file = cpymo_save_open_read(e, (unsigned short)(uintptr_t)save_id_x);
	if (file == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;
	error_t err = cpymo_save_load_savedata(e, file);
	fclose(file);
	return err;
}

error_t cpymo_save_ui_load_savedata_yesnobox(cpymo_engine * e, unsigned short save_id)
{
	char *str = NULL;
	error_t err = CPYMO_ERR_SUCC;
	const cpymo_localization *l = cpymo_localization_get(e);

	#ifndef DISABLE_AUTOSAVE
	if (save_id) 
	#endif
	{
		err = l->save_are_you_sure_load(&str, (int)save_id);
		CPYMO_THROW(err);
	}

	err = cpymo_msgbox_ui_enter(
		e,
		cpymo_str_pure(str ? str : l->save_are_you_sure_load_auto_save),
		&cpymo_save_ui_load_savedata_yesnobox_confirm,
		(void *)(uintptr_t)save_id);

	if (str) free(str);

	return err;
}


