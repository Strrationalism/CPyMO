#include "cpymo_prelude.h"
#include "cpymo_parser.h"
#include <string.h>

static char cpymo_parser_readchar(cpymo_parser *parser) 
{
	if (parser->cur_pos >= parser->stream.len) {
		parser->is_line_end = true;
		return '\0';
	}

	char ret = parser->stream.begin[parser->cur_pos];
	parser->cur_pos++;
	parser->is_line_end = false;

	if (ret == '\r') return cpymo_parser_readchar(parser);
	else { 
		if (ret == '\n') {
			parser->cur_line++;
			parser->is_line_end = true;
		}
		return ret; 
	}
}

void cpymo_parser_init(cpymo_parser *parser, const char *stream, size_t len)
{
	parser->stream.begin = stream;
	parser->stream.len = len;
	cpymo_parser_reset(parser);
}

void cpymo_parser_reset(cpymo_parser *parser)
{
	parser->cur_pos = 0;
	parser->cur_line = 0;
	parser->is_line_end = false;

	// Remove UTF8 BOM
	if (parser->stream.len >= 3) {
		if ((unsigned char)parser->stream.begin[0] == 0xEF &&
			(unsigned char)parser->stream.begin[1] == 0xBB &&
			(unsigned char)parser->stream.begin[2] == 0xBF) 
			parser->cur_pos += 3;
	}
}

bool cpymo_parser_next_line(cpymo_parser * parser)
{
	while (!parser->is_line_end) {
		const char ch = cpymo_parser_readchar(parser);

		if (ch == '\0') return false;
	}

	if (parser->cur_pos < parser->stream.len) {
		parser->is_line_end = false;
		return true;
	}
	else return false;
}

char cpymo_parser_curline_readchar(cpymo_parser * parser)
{
	if (parser->is_line_end) return '\0';
	else {
		char ch = cpymo_parser_readchar(parser);
		if (ch == '\n') return '\0';
		else return ch;
	}
}

char cpymo_parser_curline_peek(cpymo_parser * parser)
{
	char ch = cpymo_parser_curline_readchar(parser);
	parser->cur_pos--;
	return ch;
}

cpymo_str cpymo_parser_curline_readuntil(cpymo_parser * parser, char until)
{
	return cpymo_parser_curline_readuntil_or(parser, until, '\0');
}

cpymo_str cpymo_parser_curline_readuntil_or(cpymo_parser * parser, char until1, char until2)
{
	return cpymo_parser_curline_readuntil_or3(parser, until1, until2, '\0');
}

cpymo_str cpymo_parser_curline_readuntil_or3(cpymo_parser * parser, char until1, char until2, char until3)
{
	cpymo_str span;
	span.begin = parser->stream.begin + parser->cur_pos;
	span.len = 0;

	char ch;
	while ((ch = cpymo_parser_curline_readchar(parser))) {
		if (ch == until1 || ch == until2 || ch == until3) break;
		span.len++;
	}

	return span;
}

cpymo_str cpymo_parser_curline_pop_commacell(cpymo_parser * parser)
{
	cpymo_str span = cpymo_parser_curline_readuntil(parser, ',');
	cpymo_str_trim(&span);

	return span;
}

cpymo_str cpymo_parser_curline_pop_command(cpymo_parser * parser)
{
	cpymo_str before_command = cpymo_parser_curline_readuntil(parser, '#');
	cpymo_str_trim(&before_command);
	if (before_command.len == 0) {
		cpymo_str command = cpymo_parser_curline_readuntil_or(parser, ' ', '\t');
		cpymo_str_trim(&command);
		return command;
	} 
	else {
		cpymo_str ret;
		ret.begin = NULL;
		ret.len = 0;
		return ret;
	}
}

