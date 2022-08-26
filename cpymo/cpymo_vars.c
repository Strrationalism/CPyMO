﻿#include "cpymo_prelude.h"
#include "cpymo_vars.h"
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

void cpymo_vars_init(cpymo_vars * out)
{
	out->locals = NULL;
	out->globals = NULL;
	out->globals_dirty = false;
}

static void cpymo_vars_free_var(struct cpymo_var * to_free)
{
	while (to_free) {
		struct cpymo_var *var_to_free = to_free;
		to_free = to_free->next;

		free(var_to_free->name);
		free(var_to_free);
	}
}

void cpymo_vars_free(cpymo_vars * to_free)
{
	cpymo_vars_free_var(to_free->locals);
	cpymo_vars_free_var(to_free->globals);
}

void cpymo_vars_clear_locals(cpymo_vars * vars)
{
	cpymo_vars_free_var(vars->locals);
	vars->locals = NULL;
}

int *cpymo_vars_access(cpymo_vars * vars, cpymo_str name, bool modify)
{
	assert(name.len >= 1);
	bool is_global_var = name.begin[0] == 'S';
	struct cpymo_var *var = is_global_var ? vars->globals : vars->locals;

	vars->globals_dirty |= is_global_var && modify;

	while (var) {
		if (cpymo_str_equals_str(name, var->name))
			return &var->val;
		else var = var->next;
	}
	
	return NULL;
}

static error_t cpymo_vars_access_create(cpymo_vars * vars, cpymo_str name, int **ptr_to_val)
{
	assert(*ptr_to_val == NULL);

	*ptr_to_val = cpymo_vars_access(vars, name, true);
	if (*ptr_to_val != NULL) return CPYMO_ERR_SUCC;
	else {
		struct cpymo_var *var = (struct cpymo_var *)malloc(sizeof(struct cpymo_var));
		if (var == NULL) return CPYMO_ERR_OUT_OF_MEM;

		var->val = 0;
		var->name = (char *)malloc(name.len + 1);
		if (var->name == NULL) {
			free(var); 
			return CPYMO_ERR_OUT_OF_MEM;
		}

		cpymo_str_copy(var->name, name.len + 1, name);

		struct cpymo_var **parent = name.begin[0] == 'S' ? &vars->globals : &vars->locals;

		var->next = *parent;
		*parent = var;
		*ptr_to_val = &var->val;

		return CPYMO_ERR_SUCC;
	}
}

int cpymo_vars_get(cpymo_vars * vars, cpymo_str name)
{
	int *v = cpymo_vars_access(vars, name, false);
	return v == NULL ? 0 : *v;
}

error_t cpymo_vars_set(cpymo_vars *vars, cpymo_str name, int v) {
	int *p = NULL;
	error_t err = cpymo_vars_access_create(vars, name, &p);
	if (err != CPYMO_ERR_SUCC) return err;

	*p = v;
	return CPYMO_ERR_SUCC;
}

error_t cpymo_vars_add(cpymo_vars *vars, cpymo_str name, int v)
{
	return CPYMO_ERR_UNSUPPORTED;
}

bool cpymo_vars_is_constant(cpymo_str expr)
{
	for (size_t i = 0; i < expr.len; ++i) {
		if (!isdigit((int)expr.begin[i]) && expr.begin[i] != '-') {
			return false;
		}
	}

	return true;
}

int cpymo_vars_eval(cpymo_vars * vars, cpymo_str expr)
{
	if (cpymo_vars_is_constant(expr)) return cpymo_str_atoi(expr);
	else return cpymo_vars_get(vars, expr);
}


