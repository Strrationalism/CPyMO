#ifndef INCLUDE_CPYMO_VAR
#define INCLUDE_CPYMO_VAR

#include "cpymo_error.h"
#include "cpymo_parser.h"

typedef int32_t cpymo_val;

typedef struct {
	void *locals, *globals;
	bool globals_dirty;
} cpymo_vars;

void cpymo_vars_init(cpymo_vars *out);
void cpymo_vars_free(cpymo_vars *to_free);

cpymo_val *cpymo_vars_access(cpymo_vars * vars, cpymo_str name);

void cpymo_vars_clear_locals(cpymo_vars *vars);

cpymo_val cpymo_vars_get(cpymo_vars * vars, cpymo_str name);

error_t cpymo_vars_set(cpymo_vars *vars, cpymo_str name, cpymo_val v);

error_t cpymo_vars_add(cpymo_vars *vars, cpymo_str name, cpymo_val v);

bool cpymo_vars_is_constant(cpymo_str expr);

cpymo_val cpymo_vars_eval(cpymo_vars *vars, cpymo_str expr);

size_t cpymo_vars_count(void **p_var_field);

const char *cpymo_vars_get_by_index(
	void *var_field, size_t index, cpymo_val *value);

#endif