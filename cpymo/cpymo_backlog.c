#include "cpymo_backlog.h"
#include "cpymo_engine.h"
#include "cpymo_list_ui.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

void cpymo_backlog_init(cpymo_backlog *b)
{
	b->next_record_to_write = 0;
	b->pending_vo_filename[0] = '\0';
	for (size_t i = 0; i < CPYMO_BACKLOG_MAX_RECORDS; i++) {
		b->records[i].vo_filename[0] = '\0';
	}
}

void cpymo_backlog_free(cpymo_backlog *b)
{
}

void cpymo_backlog_record_on_vo(cpymo_backlog *b, cpymo_parser_stream_span vo)
{
	cpymo_parser_stream_span_copy(
		b->pending_vo_filename, sizeof(b->pending_vo_filename), vo);
}

error_t cpymo_backlog_record_write(cpymo_backlog *b, cpymo_backend_text name)
{
	cpymo_backlog_record *rec = &b->records[b->next_record_to_write];
	
	strcpy(rec->vo_filename, b->pending_vo_filename);
	b->pending_vo_filename[0] = '\0';

	b->next_record_to_write = (b->next_record_to_write + 1) % CPYMO_BACKLOG_MAX_RECORDS;
	return CPYMO_ERR_SUCC;
}

#define ENC(INDEX) CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(INDEX)
#define DEC(PTR) CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(PTR)

static void cpymo_backlog_ui_draw_node(const cpymo_engine *e, const void *node_to_draw, float y)
{
	const cpymo_backlog_record *rec = &e->backlog.records[DEC(node_to_draw)];

	const float font_size = cpymo_gameconfig_font_size(&e->gameconfig);
}

static error_t cpymo_backlog_ui_ok(struct cpymo_engine *e, void *selected)
{
	const cpymo_backlog_record *rec = &e->backlog.records[DEC(selected)];
	if (rec->vo_filename[0]) {
		return cpymo_audio_vo_play(e, cpymo_parser_stream_span_pure(rec->vo_filename));
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

	return ENC(index);
}

static void *cpymo_backlog_ui_get_prev(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	size_t index = (size_t)DEC(cur);
	if (index >= CPYMO_BACKLOG_MAX_RECORDS - 1) index = 0;
	else index++;

	if (index == e->backlog.next_record_to_write) return NULL;

	return ENC(index);
}

error_t cpymo_backlog_ui_enter(cpymo_engine *e)
{
	void *out_data = NULL;

	size_t first = e->backlog.next_record_to_write;
	if (first) first--;
	else first = CPYMO_BACKLOG_MAX_RECORDS - 1;

	return cpymo_list_ui_enter(
		e,
		&out_data,
		0,
		&cpymo_backlog_ui_draw_node,
		&cpymo_backlog_ui_ok,
		&cpymo_backlog_ui_deleter,
		ENC(first),
		&cpymo_backlog_ui_get_next,
		&cpymo_backlog_ui_get_prev,
		true,
		5);
}
