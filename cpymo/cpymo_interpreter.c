#include "cpymo_interpreter.h"
#include "cpymo_engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

error_t cpymo_interpreter_goto_label(cpymo_interpreter * interpreter, const char * label)
{
	cpymo_parser_reset(&interpreter->script_parser);

	while (1) {
		cpymo_parser_stream_span command = 
			cpymo_parser_curline_pop_command(&interpreter->script_parser);

		if (cpymo_parser_stream_span_equals_str(command, "label")) {
			cpymo_parser_stream_span cur_label = 
				cpymo_parser_curline_pop_commacell(&interpreter->script_parser);

			if (cpymo_parser_stream_span_equals_str(cur_label, label)) {
				cpymo_parser_next_line(&interpreter->script_parser);
				return CPYMO_ERR_SUCC;
			}
		}

		if (!cpymo_parser_next_line(&interpreter->script_parser)) {
			return CPYMO_ERR_NOT_FOUND;
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

static error_t cpymo_interpreter_dispatch(cpymo_parser_stream_span command, cpymo_parser *parser, cpymo_engine *engine);

error_t cpymo_interpreter_execute_step(cpymo_interpreter * interpreter, cpymo_engine *engine)
{
	cpymo_parser_stream_span command =
		cpymo_parser_curline_pop_command(&interpreter->script_parser);

	error_t err = cpymo_interpreter_dispatch(command, &interpreter->script_parser, engine);
	if (err != CPYMO_ERR_SUCC) {
		cpymo_parser_next_line(&interpreter->script_parser);
		return err;
	}

	if (!cpymo_parser_next_line(&interpreter->script_parser))
		return CPYMO_ERR_NO_MORE_CONTENT;

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_interpreter_dispatch(cpymo_parser_stream_span command, cpymo_parser *parser, cpymo_engine *engine)
{
	char buf[4096];
	cpymo_parser_stream_span_copy(buf, 4096, command);

	printf("[%d: %s]", parser->cur_line, buf);

	while (!parser->is_line_end) {
		cpymo_parser_stream_span arg = cpymo_parser_curline_pop_commacell(parser);
		cpymo_parser_stream_span_copy(buf, 4096, arg);
		printf("%s;", buf);
	}

	printf("\n");

	return CPYMO_ERR_SUCC;
}