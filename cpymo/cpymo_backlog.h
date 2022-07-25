#ifndef INCLUDE_CPYMO_BACKLOG
#define INCLUDE_CPYMO_BACKLOG

#include "cpymo_parser.h"
#include "cpymo_error.h"
#include <cpymo_backend_text.h>

#ifdef __PSP__
#define CPYMO_BACKLOG_MAX_RECORDS 8
#endif

#ifndef CPYMO_BACKLOG_MAX_RECORDS
#define CPYMO_BACKLOG_MAX_RECORDS 64
#endif

typedef struct {
	bool owning_name;
	cpymo_backend_text name, *lines;
	size_t max_lines;

	char vo_filename[32];

#ifdef ENABLE_TEXT_EXTRACT
	char *text;
#endif

} cpymo_backlog_record;

typedef struct {
	cpymo_backlog_record records[CPYMO_BACKLOG_MAX_RECORDS];
	size_t next_record_to_write;
	char pending_vo_filename[32];

	bool owning_name;
	cpymo_backend_text pending_name;
	
#ifdef ENABLE_TEXT_EXTRACT
	char *pending_text;
#endif
} cpymo_backlog;

void cpymo_backlog_init(cpymo_backlog *);
void cpymo_backlog_free(cpymo_backlog *);

void cpymo_backlog_record_write_vo(
	cpymo_backlog *,
	cpymo_parser_stream_span vo);

#ifdef ENABLE_TEXT_EXTRACT
void cpymo_backlog_record_write_full_text(
	cpymo_backlog *,
	char *text);
#endif

void cpymo_backlog_record_write_name(
	cpymo_backlog *,
	cpymo_backend_text name_moveinto);

error_t cpymo_backlog_record_write_text(
	cpymo_backlog *,
	cpymo_backend_text **textlines_moveinto,
	size_t max_lines);

struct cpymo_engine;
error_t cpymo_backlog_ui_enter(struct cpymo_engine *e);

#endif
