#ifndef INCLUDE_CPYMO_BACKLOG
#define INCLUDE_CPYMO_BACKLOG

#include "cpymo_parser.h"
#include "cpymo_error.h"
#include <cpymo_backend_text.h>

#define CPYMO_BACKLOG_MAX_RECORDS 128

typedef struct {
	bool owning_name;
	cpymo_backend_text name;

	char vo_filename[32];
} cpymo_backlog_record;

typedef struct {
	cpymo_backlog_record records[CPYMO_BACKLOG_MAX_RECORDS];
	size_t next_record_to_write;
	char pending_vo_filename[32];

	bool owning_name;
	cpymo_backend_text pending_name;
} cpymo_backlog;

void cpymo_backlog_init(cpymo_backlog *);
void cpymo_backlog_free(cpymo_backlog *);

void cpymo_backlog_record_write_vo(
	cpymo_backlog *,
	cpymo_parser_stream_span vo);

void cpymo_backlog_record_write_name(
	cpymo_backlog *,
	cpymo_backend_text name_moveinto);

error_t cpymo_backlog_record_write_text(
	cpymo_backlog *,
	cpymo_backend_text *textlines_moveinto);

struct cpymo_engine;
error_t cpymo_backlog_ui_enter(struct cpymo_engine *e);

#endif
