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
	bool globals_dirty;
} cpymo_vars;

void cpymo_vars_init(cpymo_vars *out);
void cpymo_vars_free(cpymo_vars *to_free);

int *cpymo_vars_access(cpymo_vars * vars, cpymo_str name, bool modify);

void cpymo_vars_clear_locals(cpymo_vars *vars);

int cpymo_vars_get(cpymo_vars * vars, cpymo_str name);

error_t cpymo_vars_set(cpymo_vars *vars, cpymo_str name, int v);

error_t cpymo_vars_add(cpymo_vars *vars, cpymo_str name, int v);

bool cpymo_vars_is_constant(cpymo_str expr);

int cpymo_vars_eval(cpymo_vars *vars, cpymo_str expr);

#endif