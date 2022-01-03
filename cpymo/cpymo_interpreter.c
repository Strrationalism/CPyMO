#include "cpymo_interpreter.h"
#include "cpymo_engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <assert.h>
#include <ctype.h>

error_t cpymo_interpreter_init_boot(cpymo_interpreter * out, const char * start_script_name)
{
	const char *script_format =
		"#bg logo1\n"
		"#bg logo2\n"
		"#change %s";

	out->script_name[0] = '\0';
	out->caller = NULL;
	
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

	out->caller = NULL;

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
	cpymo_interpreter *caller = interpreter->caller;
	while (caller) {
		cpymo_interpreter *to_free = caller;
		caller = caller->caller;

		free(to_free->script_content);
		free(to_free);
	}

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
		else {
			if (!cpymo_parser_next_line(&interpreter->script_parser)) {
				char label_name[32];
				cpymo_parser_stream_span_copy(label_name, sizeof(label_name), label);
				fprintf(stderr, "[Error] Can not find label %s in script %s.", label_name, interpreter->script_name);
				return CPYMO_ERR_NOT_FOUND;
			}
		}
	}
}

cpymo_interpreter_snapshot cpymo_interpreter_get_snapshot_current_callstack(const cpymo_interpreter * interpreter)
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
	error_t err;

	if (IS_EMPTY(command)) {
		CONT_NEXTLINE;
	}

	/*** I. Text ***/
	D("title") {
		POP_ARG(title);

		cpymo_parser_stream_span_trim(&title);
		char *buf = (char *)malloc(title.len + 1);
		if (buf == NULL) return CPYMO_ERR_OUT_OF_MEM;

		free(engine->title);
		engine->title = buf;
		cpymo_parser_stream_span_copy(engine->title, title.len + 1, title);
		
		CONT_NEXTLINE;
	}

	/*** III. Variables, Selection, Jump ***/
	D("set") {
		POP_ARG(name); ENSURE(name);
		POP_ARG(value_str); ENSURE(value_str);
		
		int *v = NULL;
		if ((err = cpymo_vars_access_create(&engine->vars, name, &v)) != CPYMO_ERR_SUCC)
			return err;

		*v = cpymo_parser_stream_span_atoi(value_str);
			
		CONT_NEXTLINE;
	}

	D("add") {
		POP_ARG(name); ENSURE(name);
		POP_ARG(value); ENSURE(value);

		int *v = NULL;
		if ((err = cpymo_vars_access_create(&engine->vars, name, &v)) != CPYMO_ERR_SUCC)
			return err;

		*v += cpymo_parser_stream_span_atoi(value);

		CONT_NEXTLINE;
	}

	D("sub") {
		POP_ARG(name); ENSURE(name);
		POP_ARG(value); ENSURE(value);

		int *v = NULL;
		if ((err = cpymo_vars_access_create(&engine->vars, name, &v)) != CPYMO_ERR_SUCC)
			return err;

		*v -= cpymo_parser_stream_span_atoi(value);

		CONT_NEXTLINE;
	}

	D("label") {
		CONT_NEXTLINE;
	}

	D("goto") {
		POP_ARG(label);
		ENSURE(label);
		if ((err = cpymo_interpreter_goto_label(interpreter, label)) != CPYMO_ERR_SUCC)
			return err;
		
		CONT_WITH_CURRENT_CONTEXT;
	}

	D("change") {
		POP_ARG(script_name_span);
		ENSURE(script_name_span);

		char script_name[sizeof(interpreter->script_name)];
		cpymo_parser_stream_span_copy(script_name, sizeof(script_name), script_name_span);

		cpymo_interpreter_free(interpreter);
		if ((err = cpymo_interpreter_init_script(interpreter, script_name, &engine->assetloader)) != CPYMO_ERR_SUCC)
			return err;

		CONT_WITH_CURRENT_CONTEXT;
	}

	D("if") {
		POP_ARG(condition); ENSURE(condition);

		cpymo_parser parser;
		cpymo_parser_init(&parser, condition.begin, condition.len);

		cpymo_parser_stream_span left;
		left.begin = condition.begin;
		left.len = 0;

		while (left.len < condition.len) {
			char ch = left.begin[left.len];
			if (ch == '>' || ch == '<' || ch == '=' || ch == '!') 
				break;
			left.len++;
		}

		if (left.len >= condition.len) goto BAD_EXPRESSION;

		cpymo_parser_stream_span op;
		op.begin = left.begin + left.len;
		op.len = 1;

		if (op.begin[0] == '!') {
			op.len++;
			if (op.len + left.len >= condition.len) goto BAD_EXPRESSION;
			if (op.begin[1] != '=') goto BAD_EXPRESSION;
		}

		if ((op.begin[0] == '>' || op.begin[0] == '<')) {
			if (op.len + 1 + left.len < condition.len) {
				if (op.begin[1] == '=')
					op.len++;
			}
		}

		cpymo_parser_stream_span right;
		right.begin = op.begin + op.len;
		right.len = condition.len - op.len - left.len;

		cpymo_parser_stream_span_trim(&left);
		cpymo_parser_stream_span_trim(&op);
		cpymo_parser_stream_span_trim(&right);

		if (IS_EMPTY(left) || IS_EMPTY(right) || IS_EMPTY(op)) 
			goto BAD_EXPRESSION;

		bool is_constant = true;
		for (size_t i = 0; i < right.len; ++i) {
			if (!isdigit((int)right.begin[i])) {
				is_constant = false;
				break;
			}
		}

		int lv, rv;
		{
			int *plv = cpymo_vars_access(&engine->vars, left);
			lv = plv ? *plv : 0;

			if (is_constant)
				rv = cpymo_parser_stream_span_atoi(right);
			else {
				int *prv = cpymo_vars_access(&engine->vars, right);
				rv = prv ? *prv : 0;
			}
		}

		bool run_sub_command;
		if (cpymo_parser_stream_span_equals_str(op, "="))
			run_sub_command = lv == rv;
		else if (cpymo_parser_stream_span_equals_str(op, "!="))
			run_sub_command = lv != rv;
		else if (cpymo_parser_stream_span_equals_str(op, ">"))
			run_sub_command = lv > rv;
		else if (cpymo_parser_stream_span_equals_str(op, ">="))
			run_sub_command = lv >= rv;
		else if (cpymo_parser_stream_span_equals_str(op, "<"))
			run_sub_command = lv < rv;
		else if (cpymo_parser_stream_span_equals_str(op, "<="))
			run_sub_command = lv <= rv;
		else goto BAD_EXPRESSION;

		if (run_sub_command) {
			cpymo_parser_stream_span sub_command =
				cpymo_parser_curline_readuntil_or(&interpreter->script_parser, ' ', '\t');

			cpymo_parser_stream_span_trim(&sub_command);
			return cpymo_interpreter_dispatch(sub_command, interpreter, engine, cont);
		}
		

		CONT_NEXTLINE;

		BAD_EXPRESSION: {
			char *condition_str = (char *)malloc(condition.len + 1);
			if (condition_str == NULL) return CPYMO_ERR_OUT_OF_MEM;
			cpymo_parser_stream_span_copy(condition_str, condition.len + 1, condition);
			fprintf(
				stderr, 
				"[Error] Bad if expression \"%s\" in script %s(%u).", 
				condition_str,
				interpreter->script_name,
				(unsigned)interpreter->script_parser.cur_line);
			free(condition_str);
			return CPYMO_ERR_INVALID_ARG;
		}
	}

	D("call") {
		POP_ARG(script_name_span);
		ENSURE(script_name_span);
		cpymo_parser_next_line(&interpreter->script_parser);

		char script_name[sizeof(interpreter->script_name)];
		cpymo_parser_stream_span_copy(script_name, sizeof(script_name), script_name_span);

		cpymo_interpreter *callee = (cpymo_interpreter *)malloc(sizeof(cpymo_interpreter));
		if (callee == NULL) return CPYMO_ERR_OUT_OF_MEM;

		if ((err = cpymo_interpreter_init_script(callee, script_name, &engine->assetloader)) != CPYMO_ERR_SUCC) {
			free(callee);
			return err;
		}

		assert(engine->interpreter == interpreter);

		engine->interpreter = callee;
		callee->caller = interpreter;

		return cpymo_interpreter_execute_step(callee, engine);
	}

	D("ret") {
		if (interpreter->caller == NULL) return CPYMO_ERR_NO_MORE_CONTENT;

		assert(engine->interpreter == interpreter);

		cpymo_interpreter *caller = interpreter->caller;

		engine->interpreter = interpreter->caller;
		free(interpreter->script_content);
		free(interpreter);

		return cpymo_interpreter_execute_step(caller, engine);
	}

	D("rand") {
		POP_ARG(var_name); ENSURE(var_name);
		POP_ARG(min_val_str); ENSURE(min_val_str);
		POP_ARG(max_val_str); ENSURE(max_val_str);

		int min_val = cpymo_parser_stream_span_atoi(min_val_str);
		int max_val = cpymo_parser_stream_span_atoi(max_val_str);

		if (max_val - min_val <= 0) {
			fprintf(
				stderr,
				"[Error] In script %s(%u), max value must bigger than min value for rand command.",
				interpreter->script_name,
				(unsigned)interpreter->script_parser.cur_line);

			return CPYMO_ERR_INVALID_ARG;
		}

		int *p = NULL;
		if ((err = cpymo_vars_access_create(&engine->vars, var_name, &p)) != CPYMO_ERR_SUCC)
			return err;

		*p = min_val + rand() % (max_val - min_val + 1);

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
			(unsigned)interpreter->script_parser.cur_line + 1);

		CONT_NEXTLINE;
	}
}