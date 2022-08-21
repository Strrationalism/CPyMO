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

void cpymo_vars_clear_locals(cpymo_vars *vars);

int *cpymo_vars_access(cpymo_vars *vars, cpymo_string name, bool modify);

static inline int cpymo_vars_get(cpymo_vars * vars, cpymo_string name)
{
	int *v = cpymo_vars_access(vars, name, false);
	return v == NULL ? 0 : *v;
}

error_t cpymo_vars_access_create(cpymo_vars *vars, cpymo_string name, int **ptr_to_val);

static inline error_t cpymo_vars_set(cpymo_vars *vars, cpymo_string name, int v) {
	int *p = NULL;
	error_t err = cpymo_vars_access_create(vars, name, &p);
	if (err != CPYMO_ERR_SUCC) return err;

	*p = v;
	return CPYMO_ERR_SUCC;
}

bool cpymo_vars_is_constant(cpymo_string expr);

int cpymo_vars_eval(cpymo_vars *vars, cpymo_string expr);

#endif