#ifndef INCLUDE_CPYMO_INTERPRETER
#define INCLUDE_CPYMO_INTERPRETER

#include "cpymo_parser.h"
#include "cpymo_error.h"
#include "cpymo_assetloader.h"

struct cpymo_engine;

struct cpymo_interpreter {
	char script_name[64];
	char *script_content;
	cpymo_parser script_parser;

	struct cpymo_interpreter *caller;

	struct {
		size_t cur_pos;
	} checkpoint;
};

typedef struct cpymo_interpreter cpymo_interpreter;

error_t cpymo_interpreter_init_boot(cpymo_interpreter *out, const char *start_script_name);
error_t cpymo_interpreter_init_script(cpymo_interpreter *out, const char *script_name, const cpymo_assetloader *loader);
void cpymo_interpreter_free(cpymo_interpreter *interpreter);

error_t cpymo_interpreter_goto_label(cpymo_interpreter *interpreter, cpymo_parser_stream_span label);
error_t cpymo_interpreter_execute_step(cpymo_interpreter *interpreter, struct cpymo_engine *engine);

void cpymo_interpreter_checkpoint(cpymo_interpreter *interpreter);

#endif
