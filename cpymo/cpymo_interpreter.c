#include "cpymo_interpreter.h"
#include "cpymo_engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

error_t cpymo_interpreter_init_boot(cpymo_interpreter * out, const char * start_script_name)
{
	const char *script_format =
		"#bg logo1\n"
		"#bg logo2\n"
		"#change %s";

	out->script_name[0] = '\0';
	
	size_t script_len = strlen(script_format) + 64;
	out->script_content = (char *)malloc(script_len);

	if (out->script_content == NULL || strlen(start_script_name) >= 63)
		return CPYMO_ERR_OUT_OF_MEM;

	if (sprintf(out->script_content, script_format, start_script_name) < 0) {
		free(out->script_content);
		return CPYMO_ERR_UNKNOWN;
	}

	cpymo_parser_init(&out->script_parser, out->script_content, strlen(out->script_content));
	return CPYMO_ERR_SUCC;
}

error_t cpymo_interpreter_init_script(cpymo_interpreter * out, const char * script_name, const cpymo_assetloader *loader)
{
	if (strlen(script_name) >= 63) return CPYMO_ERR_OUT_OF_MEM;
	strcpy(out->script_name, script_name);

	out->script_content = NULL;
	size_t script_len = 0;

	error_t err =
		cpymo_assetloader_load_script(&out->script_content, &script_len, script_name, loader);

	if (err != CPYMO_ERR_SUCC) return err;

	cpymo_parser_init(&out->script_parser, out->script_content, script_len);
	return CPYMO_ERR_SUCC;
}

error_t cpymo_interpreter_init_snapshot(cpymo_interpreter * out, const cpymo_interpreter_snapshot * snapshot, const cpymo_assetloader * loader)
{
	error_t err = cpymo_interpreter_init_script(out, snapshot->script_name, loader);
	if (err != CPYMO_ERR_SUCC) return err;

	return cpymo_interpreter_goto_line(out, snapshot->cur_line);
}

void cpymo_interpreter_free(cpymo_interpreter * interpreter)
{
	free(interpreter->script_content);
}

error_t cpymo_interpreter_goto_line(cpymo_interpreter * interpreter, uint64_t line)
{
	while (line != interpreter->script_parser.cur_line)
		if (!cpymo_parser_next_line(&interpreter->script_parser))
			return CPYMO_ERR_BAD_FILE_FORMAT;
	return CPYMO_ERR_SUCC;
}

error_t cpymo_interpreter_goto_label(cpymo_interpreter * interpreter, cpymo_parser_stream_span label)
{
	cpymo_parser_reset(&interpreter->script_parser);

	while (1) {
		cpymo_parser_stream_span command = 
			cpymo_parser_curline_pop_command(&interpreter->script_parser);

		if (cpymo_parser_stream_span_equals_str(command, "label")) {
			cpymo_parser_stream_span cur_label = 
				cpymo_parser_curline_pop_commacell(&interpreter->script_parser);

			if (cpymo_parser_stream_span_equals(cur_label, label)) {
				cpymo_parser_next_line(&interpreter->script_parser);
				return CPYMO_ERR_SUCC;
			}
		}
	}
}

cpymo_interpreter_snapshot cpymo_interpreter_get_snapshot(const cpymo_interpreter * interpreter)
{
	cpymo_interpreter_snapshot out;
	strcpy(out.script_name, interpreter->script_name);
	out.cur_line = interpreter->script_parser.cur_line;
	return out;
}

static error_t cpymo_interpreter_dispatch(cpymo_parser_stream_span command, cpymo_interpreter *interpreter, cpymo_engine *engine, jmp_buf cont);

error_t cpymo_interpreter_execute_step(cpymo_interpreter * interpreter, cpymo_engine *engine)
{
	jmp_buf cont;
	setjmp(cont);

	cpymo_parser_stream_span command =
		cpymo_parser_curline_pop_command(&interpreter->script_parser);

	error_t err = cpymo_interpreter_dispatch(command, interpreter, engine, cont);
	if (err != CPYMO_ERR_SUCC) {
		cpymo_parser_next_line(&interpreter->script_parser);
		return err;
	}

	if (!cpymo_parser_next_line(&interpreter->script_parser))
		return CPYMO_ERR_NO_MORE_CONTENT;

	return CPYMO_ERR_SUCC;
}

#define D(CMD) \
	else if (cpymo_parser_stream_span_equals_str(command, CMD))

#define POP_ARG(X) \
	cpymo_parser_stream_span X = cpymo_parser_curline_pop_commacell(&interpreter->script_parser); \
	cpymo_parser_stream_span_trim(&X)

#define IS_EMPTY(X) \
	cpymo_parser_stream_span_equals_str(X, "")
	
#define ENSURE(X) \
	{ if (IS_EMPTY(X)) return CPYMO_ERR_INVALID_ARG; }

#define CONT_WITH_CURRENT_CONTEXT { longjmp(cont, 1); return CPYMO_ERR_UNKNOWN; }

#define CONT_NEXTLINE { \
	if (cpymo_parser_next_line(&interpreter->script_parser))	\
		{ longjmp(cont, 1); return CPYMO_ERR_UNKNOWN; }	\
	else return CPYMO_ERR_NO_MORE_CONTENT; }

static error_t cpymo_interpreter_dispatch(cpymo_parser_stream_span command, cpymo_interpreter *interpreter, cpymo_engine *engine, jmp_buf cont)
{
	if (IS_EMPTY(command)) {
		CONT_NEXTLINE;
	}

	D("goto") {
		POP_ARG(label);
		ENSURE(label);
		cpymo_interpreter_goto_label(interpreter, label);
		
		CONT_WITH_CURRENT_CONTEXT;
	}

	D("change") {
		POP_ARG(script_name_span);
		ENSURE(script_name_span);

		char script_name[sizeof(interpreter->script_name)];
		cpymo_parser_stream_span_copy(script_name, sizeof(script_name), script_name_span);

		cpymo_interpreter_free(interpreter);
		cpymo_interpreter_init_script(interpreter, script_name, &engine->assetloader);

		CONT_WITH_CURRENT_CONTEXT;
	}

	D("label") {
		CONT_NEXTLINE;
	}
	
	else {
		char buf[32];
		cpymo_parser_stream_span_copy(buf, 32, command);

		fprintf(
			stderr,
			"[Warning] Unknown command \"%s\" in script %s(%u).\n",
			buf,
			interpreter->script_name,
			interpreter->script_parser.cur_line + 1);

		CONT_NEXTLINE;
	}
}