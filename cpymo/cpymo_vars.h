#ifndef INCLUDE_CPYMO_VAR
#define INCLUDE_CPYMO_VAR

#include "cpymo_error.h"
#include "cpymo_parser.h"

struct cpymo_var {
	struct cpymo_var *next;
	char *name;
	int val;
};

typedef struct {
	struct cpymo_var *locals;
	struct cpymo_var *globals;
} cpymo_vars;

void cpymo_vars_init(cpymo_vars *out);
void cpymo_vars_free(cpymo_vars *to_free);

void cpymo_vars_clear_locals(cpymo_vars *vars);

int *cpymo_vars_access(cpymo_vars *vars, cpymo_parser_stream_span name);
error_t cpymo_vars_access_create(cpymo_vars *vars, cpymo_parser_stream_span name, int **ptr_to_val);

#endif