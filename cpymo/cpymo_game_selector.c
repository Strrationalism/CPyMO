#include "cpymo_game_selector.h"
#include "cpymo_engine.h"
#include "cpymo_list_ui.h"
#include "cpymo_localization.h"
#include <stb_image.h>
#include <assert.h>
#include <string.h>

typedef struct {
	cpymo_game_selector_item *items;
	cpymo_game_selector_callback before_init, after_init;
	size_t draw_times;
} cpymo_game_selector;

error_t cpymo_game_selector_init_refresh(cpymo_engine *e, float dt, void *selected)
{
	cpymo_game_selector *ui = (cpymo_game_selector *)cpymo_list_ui_data(e);
	cpymo_engine_request_redraw(e);

	if (ui->draw_times) {
		ui->draw_times--;
	}
	else {
		cpymo_list_ui_set_custom_update(e, NULL);
	}

	return CPYMO_ERR_SUCC;
}

static void cpymo_game_selector_draw_node(const cpymo_engine *e, const void *node_to_draw, float y)
{
	cpymo_game_selector_item *item = (cpymo_game_selector_item *)node_to_draw;
	
	float game_icon_size = 3 * (float)e->gameconfig.fontsize;
	if (item->icon) {
		cpymo_backend_image_draw(16, y + 6, game_icon_size, game_icon_size, item->icon, 0, 0,
			item->icon_w, item->icon_h, 1.0f, cpymo_backend_image_draw_type_ui_element);
	}

	assert(item->gametitle);
	cpymo_backend_text_draw(
		item->gametitle, 28 + game_icon_size, y + (float)e->gameconfig.fontsize, cpymo_color_white, 1.0f,
		cpymo_backend_image_draw_type_ui_element);
}

static error_t cpymo_game_selector_ok(cpymo_engine *e, void *selected)
{
	cpymo_game_selector_item *item = (cpymo_game_selector_item *)selected;
	char *gamedir = item->gamedir;
	item->gamedir = NULL;

	cpymo_game_selector *sel = (cpymo_game_selector *)cpymo_list_ui_data(e);

	if (sel->before_init) {
		error_t err = sel->before_init(e, gamedir);
		if (err != CPYMO_ERR_SUCC) {
			free(gamedir);
			return err;
		}
	}

	cpymo_game_selector_callback after = sel->after_init;

	cpymo_engine_free(e);
	error_t err = cpymo_engine_init(e, gamedir);
	free(gamedir);

	CPYMO_THROW(err);

	if (after) {
		err = after(e, gamedir);
		CPYMO_THROW(err);
	}
	
	return err;
}

static void cpymo_game_selector_delete(cpymo_engine *e, void *ui_data)
{
	cpymo_game_selector *ui = (cpymo_game_selector *)ui_data;
	cpymo_game_selector_item_free_all(ui->items);
}

static void *cpymo_game_selector_get_next(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	const cpymo_game_selector_item *item = (const cpymo_game_selector_item *)cur;
	return item->next;
}

static void *cpymo_game_selector_get_prev(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	const cpymo_game_selector_item *item = (const cpymo_game_selector_item *)cur;
	return item->prev;
}

static void cpymo_game_selector_item_load_info(cpymo_game_selector_item *item, float fontsize)
{
#define FAIL { if (item->gamedir) free(item->gamedir); item->gamedir = NULL; prev = item; item = item->next; continue; }

	cpymo_game_selector_item *prev = NULL;
	char *path = NULL;
	while (item) {
		item->prev = prev;
		char *next_path = (char *)realloc(path, strlen(item->gamedir) + 24);
		if (next_path == NULL) FAIL;

		path = next_path;

		sprintf(path, "%s/gameconfig.txt", item->gamedir);

		cpymo_gameconfig game_config;
		error_t err = cpymo_gameconfig_parse_from_file(&game_config, path);
		if (err != CPYMO_ERR_SUCC) FAIL;

		cpymo_backend_text_create(&item->gametitle, &item->gametitle_w,
			cpymo_parser_stream_span_pure(game_config.gametitle), fontsize);

		sprintf(path, "%s/icon.png", item->gamedir);
		int c;
		stbi_uc *pixels = stbi_load(path, &item->icon_w, &item->icon_h, &c, 4);
		if (pixels) {
			err = cpymo_backend_image_load(&item->icon, pixels, item->icon_w, item->icon_h, cpymo_backend_image_format_rgba);
			if (err != CPYMO_ERR_SUCC) {
				free(pixels);
				item->icon = NULL;
			}
		}

		prev = item;
		item = item->next;
	}

	if (path) free(path);
}

static cpymo_game_selector_item *cpymo_game_selector_item_remove_invalid(cpymo_game_selector_item *prev, cpymo_game_selector_item *cur)
{
	if (cur == NULL) return NULL;
	else if (cur->gamedir) {
		cpymo_game_selector_item_remove_invalid(cur, cur->next);
		return cur;
	}
	else {
		cpymo_game_selector_item *next = cur->next;
		if (cur->gametitle) cpymo_backend_text_free(cur->gametitle);
		if (cur->icon) cpymo_backend_image_free(cur->icon);
		free(cur);

		if (next) next->prev = prev;
		if (prev) prev->next = next;

		return cpymo_game_selector_item_remove_invalid(prev, next);
	}
}

typedef struct {
	cpymo_backend_text msg1, msg2;
	float msg1_w, msg2_w;
	size_t draw_times;
	float font_size;
} cpymo_game_selector_empty_ui;

static void cpymo_game_selector_empty_ui_draw(const cpymo_engine *e, const void *ui_data)
{
	cpymo_game_selector_empty_ui *ui = (cpymo_game_selector_empty_ui *)ui_data;

	float fontsize = ui->font_size;

	cpymo_backend_text_draw(
		ui->msg1, 
		(e->gameconfig.imagesize_w - ui->msg1_w) / 2, 
		(e->gameconfig.imagesize_h - fontsize) / 2,
		cpymo_color_white, 0.5f,
		cpymo_backend_image_draw_type_ui_element);

	cpymo_backend_text_draw(
		ui->msg2, 
		(e->gameconfig.imagesize_w - ui->msg2_w) / 2, 
		(e->gameconfig.imagesize_h - fontsize) / 2 + fontsize + fontsize / 2,
		cpymo_color_white, 0.25f,
		cpymo_backend_image_draw_type_ui_element);
}

static error_t cpymo_game_selector_empty_ui_update(cpymo_engine *e, void *ui_data, float dt)
{
	cpymo_game_selector_empty_ui *ui = (cpymo_game_selector_empty_ui *)ui_data;
	if (ui->draw_times) {
		cpymo_engine_request_redraw(e);
		ui->draw_times--;
	}
	return CPYMO_ERR_SUCC;
}

static void cpymo_game_selector_empty_ui_delete(cpymo_engine *e, void *ui_)
{
	cpymo_game_selector_empty_ui *ui = (cpymo_game_selector_empty_ui *)ui_;
	if (ui->msg1) cpymo_backend_text_free(ui->msg1);
	if (ui->msg2) cpymo_backend_text_free(ui->msg2);
}

typedef struct {
	cpymo_game_selector_item *items;
	cpymo_game_selector_callback before_reinit, after_reinit;
	float empty_message_font_size;
	size_t nodes_per_screen;
	char *last_selected_gamedir;
} cpymo_game_selector_lazy_init;

static void cpymo_game_selector_lazy_init_delete(cpymo_engine *e, void *ui_data)
{
	cpymo_game_selector_lazy_init *ui = (cpymo_game_selector_lazy_init *)ui_data;
	if (ui->items) cpymo_game_selector_item_free_all(ui->items);
	if (ui->last_selected_gamedir) free(ui->last_selected_gamedir);
}

static void cpymo_game_selector_lazy_init_draw(const cpymo_engine *e, const void *_) {}

static error_t cpymo_game_selector_lazy_init_update(cpymo_engine *e, void *ui_, float _)
{
	float fontsize = (float)e->gameconfig.fontsize;
	cpymo_game_selector_lazy_init *data = (cpymo_game_selector_lazy_init *)ui_;

	cpymo_game_selector_item_load_info(data->items, fontsize);
	data->items = cpymo_game_selector_item_remove_invalid(NULL, data->items);

	cpymo_game_selector_item *last_selected = data->items;
	while (last_selected) {
		if (strcmp(last_selected->gamedir, data->last_selected_gamedir) == 0) break;
		last_selected = last_selected->next;
	}

	if (data->items) {
		cpymo_game_selector *ui = NULL;
		cpymo_game_selector_item *items = data->items;
		data->items = NULL;
		cpymo_game_selector_callback
			after_reinit = data->after_reinit,
			before_reinit = data->before_reinit;

		int relative = 0;
		if (last_selected) {
			if (last_selected->prev) {
				last_selected = last_selected->prev;
				relative = 1;
			}
		}

		error_t err = cpymo_list_ui_enter(
			e,
			(void **)&ui,
			sizeof(cpymo_game_selector),
			&cpymo_game_selector_draw_node,
			&cpymo_game_selector_ok,
			&cpymo_game_selector_delete,
			last_selected ? last_selected : items,
			&cpymo_game_selector_get_next,
			&cpymo_game_selector_get_prev,
			false,
			data->nodes_per_screen
		);
		CPYMO_THROW(err);

		cpymo_list_ui *list_ui = (cpymo_list_ui *)cpymo_ui_data(e);
		list_ui->selection_relative_to_cur = relative;

		cpymo_list_ui_set_custom_update(e, &cpymo_game_selector_init_refresh);
		cpymo_list_ui_enable_loop(e);

		ui->after_init = after_reinit;
		ui->before_init = before_reinit;
		ui->draw_times = 45;

		if (err != CPYMO_ERR_SUCC) {
			cpymo_game_selector_item_free_all(items);
			return err;
		}

		ui->items = items;
		return CPYMO_ERR_SUCC;
	}
	else {
		cpymo_game_selector_empty_ui *ui = NULL;
		float font_size = data->empty_message_font_size;
		error_t err = cpymo_ui_enter(
			(void **)&ui,
			e,
			sizeof(*ui),
			&cpymo_game_selector_empty_ui_update,
			&cpymo_game_selector_empty_ui_draw,
			&cpymo_game_selector_empty_ui_delete);

		if (err != CPYMO_ERR_SUCC) {
			cpymo_engine_free(e);
			return err;
		}

		ui->msg1 = NULL;
		ui->msg2 = NULL;
		ui->msg1_w = 0;
		ui->msg2_w = 0;
		ui->draw_times = 45;
		ui->font_size = font_size;

		const cpymo_localization *l = cpymo_localization_get(e);

		err = cpymo_backend_text_create(&ui->msg1, &ui->msg1_w,
			cpymo_parser_stream_span_pure(l->game_selector_empty),
			font_size);

		if (err != CPYMO_ERR_SUCC) {
			cpymo_engine_free(e);
			return err;
		}

		err = cpymo_backend_text_create(&ui->msg2, &ui->msg2_w,
			cpymo_parser_stream_span_pure(l->game_selector_empty_secondary),
			font_size / 1.3f);

		if (err != CPYMO_ERR_SUCC) {
			cpymo_engine_free(e);
			return err;
		}

		return CPYMO_ERR_SUCC;
	}
}

error_t cpymo_engine_init_with_game_selector(
	cpymo_engine * e, size_t screen_w, size_t screen_h, size_t fontsize_, 
	float empty_message_font_size,
	size_t nodes_per_screen, cpymo_game_selector_item **gamedirs_movein,
	cpymo_game_selector_callback before_reinit,
	cpymo_game_selector_callback after_reinit,
	char **last_selected_game_dir_movein)
{
	cpymo_audio_init(&e->audio);

	error_t err = cpymo_gameconfig_parse(&e->gameconfig, "", 0);
	CPYMO_THROW(err);

	e->gameconfig.imagesize_w = (uint16_t)screen_w;
	e->gameconfig.imagesize_h = (uint16_t)screen_h;
	e->gameconfig.fontsize = (uint16_t)fontsize_;

	e->assetloader.use_pkg_bg = false;
	e->assetloader.use_pkg_chara = false;
	e->assetloader.use_pkg_se = false;
	e->assetloader.use_pkg_voice = false;
	e->assetloader.game_config = &e->gameconfig;
	e->assetloader.gamedir = NULL;
	
	cpymo_vars_init(&e->vars);
	e->interpreter = NULL;
	e->title = NULL;

	cpymo_wait_reset(&e->wait);
	cpymo_flash_reset(&e->flash);
	cpymo_fade_reset(&e->fade);
	cpymo_bg_init(&e->bg);
	cpymo_anime_init(&e->anime);
	cpymo_select_img_init(&e->select_img);
	cpymo_charas_init(&e->charas);
	cpymo_scroll_init(&e->scroll);
	cpymo_floating_hint_init(&e->floating_hint);
	cpymo_say_init(&e->say);
	cpymo_text_init(&e->text);
	cpymo_hash_flags_init(&e->flags);
	e->ui = NULL;
	cpymo_backlog_init(&e->backlog);
	e->skipping = false;
	e->redraw = true;

	e->input = e->prev_input = cpymo_input_snapshot();

	cpymo_game_selector_lazy_init *d = NULL;
	err = cpymo_ui_enter(
		(void **)&d,
		e,
		sizeof(*d),
		&cpymo_game_selector_lazy_init_update,
		&cpymo_game_selector_lazy_init_draw,
		&cpymo_game_selector_lazy_init_delete);

	if (err != CPYMO_ERR_SUCC) {
		cpymo_engine_free(e);
		return err;
	}

	d->after_reinit = after_reinit;
	d->nodes_per_screen = nodes_per_screen;
	d->items = *gamedirs_movein;
	d->before_reinit = before_reinit;
	d->empty_message_font_size = empty_message_font_size;
	d->last_selected_gamedir = *last_selected_game_dir_movein;
	*last_selected_game_dir_movein = NULL;
	*gamedirs_movein = NULL;

	return CPYMO_ERR_SUCC;
}

error_t cpymo_game_selector_item_create(cpymo_game_selector_item ** out, char **game_dir_move_in)
{
	*out = (cpymo_game_selector_item *)malloc(sizeof(cpymo_game_selector_item));
	if (*out == NULL) return CPYMO_ERR_OUT_OF_MEM;
	(*out)->prev = NULL;
	(*out)->next = NULL;
	(*out)->gamedir = *game_dir_move_in;
	*game_dir_move_in = NULL;
	(*out)->icon = NULL;
	(*out)->gametitle = NULL;
	return CPYMO_ERR_SUCC;
}

void cpymo_game_selector_item_free_all(cpymo_game_selector_item * item)
{
	while (item) {
		if (item->gamedir) free(item->gamedir);
		if (item->icon) cpymo_backend_image_free(item->icon);
		if (item->gametitle) cpymo_backend_text_free(item->gametitle);

		cpymo_game_selector_item *cur = item;
		item = item->next;
		free(cur);
	}
}
