#ifndef INCLUDE_CPYMO_BACKLOG
#define INCLUDE_CPYMO_BACKLOG

#include "cpymo_parser.h"
#include "cpymo_error.h"
#include "../cpymo-backends/include/cpymo_backend_text.h"

struct cpymo_backlog_record;

typedef struct {
	struct cpymo_backlog_record *records;
	size_t next_record_to_write;
	char pending_vo_filename[32];

	bool owning_name;
	cpymo_backend_text pending_name;

#ifdef ENABLE_TEXT_EXTRACT
	char *pending_name_str;
#endif
} cpymo_backlog;

error_t cpymo_backlog_init(cpymo_backlog *);
void cpymo_backlog_free(cpymo_backlog *);

void cpymo_backlog_record_write_vo(
	cpymo_backlog *,
	cpymo_str vo);

void cpymo_backlog_record_write_name(
	cpymo_backlog *,
	cpymo_backend_text name_moveinto,
	cpymo_str name_str);

error_t cpymo_backlog_record_write_text(
	cpymo_backlog *,
	char *text,
	float fontsize);

struct cpymo_engine;
error_t cpymo_backlog_ui_enter(struct cpymo_engine *e);

#endif
