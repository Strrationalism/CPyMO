#ifndef INCLUDE_CPYMO_PARSER
#define INCLUDE_CPYMO_PARSER

#include "cpymo_string.h"

typedef struct {
	cpymo_string stream;

	size_t cur_pos;
	size_t cur_line;
	bool is_line_end;
} cpymo_parser;

void cpymo_parser_init(cpymo_parser *parser, const char *stream, size_t len);
void cpymo_parser_reset(cpymo_parser *parser);
bool cpymo_parser_next_line(cpymo_parser *parser);

char cpymo_parser_curline_readchar(cpymo_parser *parser);
char cpymo_parser_curline_peek(cpymo_parser *parser);
cpymo_string cpymo_parser_curline_readuntil(cpymo_parser *parser, char until);
cpymo_string cpymo_parser_curline_readuntil_or(cpymo_parser *parser, char until1, char until2);
cpymo_string cpymo_parser_curline_readuntil_or3(cpymo_parser *parser, char until1, char until2, char until3);
cpymo_string cpymo_parser_curline_pop_commacell(cpymo_parser *parser);
cpymo_string cpymo_parser_curline_pop_command(cpymo_parser *parser);

#endif
