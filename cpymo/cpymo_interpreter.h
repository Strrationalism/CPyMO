#ifndef INCLUDE_CPYMO_INTERPRETER
#define INCLUDE_CPYMO_INTERPRETER

#include "cpymo_parser.h"
#include "cpymo_error.h"
#include "cpymo_assetloader.h"

struct cpymo_engine;

typedef struct {
	char script_name[64];
	uint64_t cur_line;
} cpymo_interpreter_snapshot;

struct cpymo_interpreter {
	char script_name[64];
	char *script_content;
	cpymo_parser script_parser;

	struct cpymo_interpreter *caller;
};

typedef struct cpymo_interpreter cpymo_interpreter;

error_t cpymo_interpreter_init_boot(cpymo_interpreter *out, const char *start_script_name);
error_t cpymo_interpreter_init_script(cpymo_interpreter *out, const char *script_name, const cpymo_assetloader *loader);
error_t cpymo_interpreter_init_snapshot(cpymo_interpreter *out, const cpymo_interpreter_snapshot *snapshot, const cpymo_assetloader *loader);
void cpymo_interpreter_free(cpymo_interpreter *interpreter);

error_t cpymo_interpreter_goto_line(cpymo_interpreter *interpreter, uint64_t line);
error_t cpymo_interpreter_goto_label(cpymo_interpreter *interpreter, cpymo_parser_stream_span label);
error_t cpymo_interpreter_execute_step(cpymo_interpreter *interpreter, struct cpymo_engine *engine);

cpymo_interpreter_snapshot cpymo_interpreter_get_snapshot_current_callstack(const cpymo_interpreter * interpreter);

#endif
