#include "cpymo_prelude.h"
#include "cpymo_backlog.h"
#include "cpymo_engine.h"
#include "cpymo_list_ui.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

#ifndef CPYMO_BACKLOG_MAX_RECORDS
#define CPYMO_BACKLOG_MAX_RECORDS 64
#endif

typedef struct cpymo_backlog_record {
	bool owning_name;
	cpymo_backend_text name, text_render;
	char vo_filename[32];
	char *text;
	float font_size;

#ifdef ENABLE_TEXT_EXTRACT
	char *name_str;
#endif
} cpymo_backlog_record;

error_t cpymo_backlog_init(cpymo_backlog *b)
{
	b->records = (cpymo_backlog_record *)malloc(CPYMO_BACKLOG_MAX_RECORDS * sizeof(b->records[0]));
	if (b->records == NULL) return CPYMO_ERR_OUT_OF_MEM;
	b->next_record_to_write = 0;
	b->pending_vo_filename[0] = '\0';
	b->owning_name = false;
	b->pending_name = NULL;

#ifdef ENABLE_TEXT_EXTRACT
	b->pending_name_str = NULL;
#endif

	for (size_t i = 0; i < CPYMO_BACKLOG_MAX_RECORDS; i++) {
		b->records[i].vo_filename[0] = '\0';
		b->records[i].owning_name = false;
		b->records[i].name = NULL;
		b->records[i].text = NULL;
		b->records[i].text_render = NULL;

#ifdef ENABLE_TEXT_EXTRACT
		b->records[i].name_str = NULL;
#endif
	}

	return CPYMO_ERR_SUCC;
}

static void cpymo_backlog_record_clean(cpymo_backlog_record *rec)
{
	if (rec->owning_name && rec->name) {
		cpymo_backend_text_free(rec->name);

#ifdef ENABLE_TEXT_EXTRACT
		if (rec->name_str) free(rec->name_str);
#endif
	}

	if (rec->text_render) cpymo_backend_text_free(rec->text_render);
	rec->text_render = NULL;

	if (rec->text) free(rec->text);
	rec->text = NULL;

	rec->owning_name = false;
	rec->name = NULL;
	rec->vo_filename[0] = '\0';

#ifdef ENABLE_TEXT_EXTRACT
	rec->name_str = NULL;
#endif
}

void cpymo_backlog_free(cpymo_backlog *b)
{
	if (b->owning_name && b->pending_name) {
		cpymo_backend_text_free(b->pending_name);

#ifdef ENABLE_TEXT_EXTRACT
		if (b->pending_name_str) free(b->pending_name_str);
#endif
	}

	for (size_t i = 0; i < CPYMO_BACKLOG_MAX_RECORDS; i++) {
		cpymo_backlog_record *rec = &b->records[i];
		cpymo_backlog_record_clean(rec);
	}

	free(b->records);
}

void cpymo_backlog_record_write_vo(cpymo_backlog *b, cpymo_str vo)
{
	cpymo_str_copy(
		b->pending_vo_filename, sizeof(b->pending_vo_filename), vo);
}

void cpymo_backlog_record_write_name(
	cpymo_backlog *b, cpymo_backend_text name, cpymo_str name_text)
{
	if (b->owning_name && b->pending_name) {
		cpymo_backend_text_free(b->pending_name);
#ifdef ENABLE_TEXT_EXTRACT
		if (b->pending_name_str) free(b->pending_name_str);
#endif
	}

	b->owning_name = name == NULL ? false : true;
	b->pending_name = name;

#ifdef ENABLE_TEXT_EXTRACT
	b->pending_name_str = 
		name == NULL ? NULL : cpymo_str_copy_malloc(name_text);
#endif
}

error_t cpymo_backlog_record_write_text(
	cpymo_backlog *b,
	char *text,
	float fontsize)
{
	cpymo_backlog_record *rec = &b->records[b->next_record_to_write];

	cpymo_backlog_record_clean(rec);

	rec->name = b->pending_name;
	rec->owning_name = b->owning_name;

#ifdef ENABLE_TEXT_EXTRACT
	rec->name_str = b->pending_name_str;
#endif

	b->owning_name = false;
	rec->text = text;
	strcpy(rec->vo_filename, b->pending_vo_filename);
	b->pending_vo_filename[0] = '\0';
	rec->font_size = fontsize;

	b->next_record_to_write = (b->next_record_to_write + 1) % CPYMO_BACKLOG_MAX_RECORDS;
	return CPYMO_ERR_SUCC;
}

#define ENC(INDEX) CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(INDEX)
#define DEC(PTR) CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(PTR)

static void cpymo_backlog_ui_draw_node(const cpymo_engine *e, const void *node_to_draw, float y)
{
	const cpymo_backlog_record *rec = &e->backlog.records[DEC(node_to_draw)];

	const float font_size = cpymo_gameconfig_font_size(&e->gameconfig);
	y += font_size;
	if (rec->name) {
		cpymo_backend_text_draw(
			rec->name, 0, y, cpymo_color_white,
			1.0, cpymo_backend_image_draw_type_ui_element);

		y += font_size;
	}

	if (rec->text_render == NULL) {
		float w;
		cpymo_backlog_record *rec_mut = (cpymo_backlog_record *)rec;
		error_t err = cpymo_backend_text_create(
			&rec_mut->text_render, 
			&w, 
			cpymo_str_pure(rec->text), 
			rec->font_size);
		if (err != CPYMO_ERR_SUCC) 
			rec_mut->text_render = NULL;
	}

	if (rec->text_render) 
		cpymo_backend_text_draw(
			rec->text_render, 
			0, y, 
			cpymo_color_white, 
			1, 
			cpymo_backend_image_draw_type_ui_element);
}

static error_t cpymo_backlog_ui_ok(struct cpymo_engine *e, void *selected)
{
	const cpymo_backlog_record *rec = &e->backlog.records[DEC(selected)];
	if (rec->vo_filename[0]) {
		return cpymo_audio_vo_play(e, cpymo_str_pure(rec->vo_filename));
	}

	return CPYMO_ERR_SUCC;
}

static void cpymo_backlog_ui_deleter(cpymo_engine *e, void *ui_)
{
}

static void *cpymo_backlog_ui_get_next(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	size_t index = (size_t)DEC(cur);
	if (index) index--;
	else index = CPYMO_BACKLOG_MAX_RECORDS - 1;

	if (index == e->backlog.next_record_to_write) return NULL;
	if (e->backlog.records[index].text == NULL) return NULL;

	return ENC(index);
}

static void *cpymo_backlog_ui_get_prev(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	size_t index = (size_t)DEC(cur);
	if (index >= CPYMO_BACKLOG_MAX_RECORDS - 1) index = 0;
	else index++;

	if (index == e->backlog.next_record_to_write) return NULL;
	if (e->backlog.records[index].text == NULL) return NULL;

	return ENC(index);
}

typedef struct {
	bool press_key_down_to_close;
} cpymo_backlog_ui;

static error_t cpymo_backlog_ui_update(cpymo_engine *e, float dt, void *selected)
{
	cpymo_backlog_ui *ui = (cpymo_backlog_ui * )cpymo_list_ui_data(e);
	if (cpymo_backlog_ui_get_prev(e, cpymo_list_ui_data(e), selected) == NULL) {
		if (CPYMO_INPUT_JUST_RELEASED(e, down)) {
			if (ui->press_key_down_to_close) cpymo_list_ui_exit(e);
			else ui->press_key_down_to_close = true;
		}
	}
	else {
		ui->press_key_down_to_close = false;
	}

	return CPYMO_ERR_SUCC;
}

#ifdef ENABLE_TEXT_EXTRACT
static error_t cpymo_backlog_ui_selection_changed(cpymo_engine *e, void *selected)
{
	if (selected) {
		size_t index = (size_t)DEC(selected);
		if (e->backlog.records[index].text) {
			if (e->backlog.records[index].name_str) {
				char *extract_text = (char *)malloc(
					strlen(e->backlog.records[index].name_str) + 2 +
					strlen(e->backlog.records[index].text));
				sprintf(extract_text, "%s\n%s",
					e->backlog.records[index].name_str,
					e->backlog.records[index].text);
				cpymo_backend_text_extract(extract_text);
				free(extract_text);
			}
			else {
				cpymo_backend_text_extract(e->backlog.records[index].text);
			}
		}
	}
	return CPYMO_ERR_SUCC;
}
#endif

error_t cpymo_backlog_ui_enter(cpymo_engine *e)
{
	cpymo_backlog_ui *ui = NULL;

	size_t first = e->backlog.next_record_to_write;
	if (first) first--;
	else first = CPYMO_BACKLOG_MAX_RECORDS - 1;

	error_t err = cpymo_list_ui_enter(
		e,
		(void **)&ui,
		sizeof(cpymo_backlog_ui),
		&cpymo_backlog_ui_draw_node,
		&cpymo_backlog_ui_ok,
		&cpymo_backlog_ui_deleter,
		ENC(first),
		&cpymo_backlog_ui_get_next,
		&cpymo_backlog_ui_get_prev,
		true,
		3);
	CPYMO_THROW(err);

	cpymo_list_ui_set_custom_update(e, &cpymo_backlog_ui_update);

#ifdef ENABLE_TEXT_EXTRACT
	cpymo_list_ui_set_selection_changed_callback(
		e, &cpymo_backlog_ui_selection_changed);
	if (e->backlog.records[first].name_str) {
		char *extract_text = (char *)malloc(
			strlen(e->backlog.records[first].name_str) + 2 +
			strlen(e->backlog.records[first].text));
		sprintf(extract_text, "%s\n%s",
			e->backlog.records[first].name_str,
			e->backlog.records[first].text);
		cpymo_backend_text_extract(extract_text);
		free(extract_text);
	}
	else {
		cpymo_backend_text_extract(e->backlog.records[first].text);
	}
#endif

	ui->press_key_down_to_close = true;

	return CPYMO_ERR_SUCC;
}

