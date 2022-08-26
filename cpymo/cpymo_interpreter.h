#ifndef INCLUDE_CPYMO_INTERPRETER
#define INCLUDE_CPYMO_INTERPRETER

#include "cpymo_parser.h"
#include "cpymo_error.h"
#include "cpymo_assetloader.h"
#include "cpymo_script.h"

struct cpymo_engine;

struct cpymo_interpreter {
	cpymo_script *script;
	bool own_script;

	cpymo_parser script_parser;

	bool no_more_content;

	struct cpymo_interpreter *caller;

	struct {
		size_t cur_line;
	} checkpoint;
};

typedef struct cpymo_interpreter cpymo_interpreter;

void cpymo_interpreter_init(
	cpymo_interpreter *out, 
	cpymo_script *script, 
	bool own_script);
	
error_t cpymo_interpreter_init_script(
	cpymo_interpreter *out, 
	cpymo_str script_name, 
	const cpymo_assetloader *loader);

void cpymo_interpreter_free(cpymo_interpreter *interpreter);

error_t cpymo_interpreter_goto_label(cpymo_interpreter *interpreter, cpymo_str label);
error_t cpymo_interpreter_execute_step(cpymo_interpreter *interpreter, struct cpymo_engine *engine);

void cpymo_interpreter_checkpoint(cpymo_interpreter *interpreter);

error_t cpymo_interpreter_goto_line(cpymo_interpreter *interpreter, uint64_t line);

#endif
