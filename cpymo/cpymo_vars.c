#include "cpymo_prelude.h"
#include "cpymo_vars.h"
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include "../stb/stb_ds.h"

struct cpymo_var {
    char *key;
    cpymo_val value;
};

void cpymo_vars_init(cpymo_vars *out)
{
    struct cpymo_var *p = NULL;
    sh_new_arena(p);
    out->locals = (void *)p;

    p = NULL;
    sh_new_arena(p);
    out->globals = (void *)p;

    out->globals_dirty = false;
}

void cpymo_vars_free(cpymo_vars *to_free)
{    
    struct cpymo_var *p = (struct cpymo_var *)to_free->globals;
    if (p) shfree(p);

    p = (struct cpymo_var *)to_free->locals;
    if (p) shfree(p);
}

static inline bool cpymo_vars_is_global(const char *name)
{ return name[0] == 'S'; }

static cpymo_val *cpymo_vars_access_cstr(
    cpymo_vars *vars, const char *name)
{
    bool is_global = cpymo_vars_is_global(name);
    struct cpymo_var **var_slot = 
        (struct cpymo_var **)(is_global ? &vars->globals : &vars->locals);

    struct cpymo_var *hash_table = *var_slot;
    
    struct cpymo_var *r = shgetp_null(hash_table, name);
    *var_slot = hash_table;

    return r ? &r->value : NULL;
}

const cpymo_val *cpymo_vars_access(cpymo_vars *vars, cpymo_str name)
{
    char *cstr = cpymo_str_copy_malloc(name);
    if (cstr == NULL) return NULL;

    cpymo_val *r = cpymo_vars_access_cstr(vars, cstr);
    free(cstr);
    return r;
}

void cpymo_vars_clear_locals(cpymo_vars *vars)
{
    struct cpymo_var *v = (struct cpymo_var *)vars->locals;
    shfree(v);

    v = NULL;
    sh_new_arena(v);
    vars->locals = (void *)v;
}

static error_t cpymo_vars_set_cstr(
    cpymo_vars *vars, const char *name, cpymo_val v)
{
    bool is_global = cpymo_vars_is_global(name);
    struct cpymo_var **var_slot = 
        (struct cpymo_var **)
        (is_global ? &vars->globals : &vars->locals);

    struct cpymo_var *hash_table = *var_slot;
    shput(hash_table, name, v);
    *var_slot = hash_table;

    if (is_global) 
        vars->globals_dirty = true;

    return CPYMO_ERR_SUCC;
}

error_t cpymo_vars_set(cpymo_vars *vars, cpymo_str name, cpymo_val v)
{
    char *name_cstr = cpymo_str_copy_malloc(name);
    if (name_cstr == NULL) return CPYMO_ERR_OUT_OF_MEM;

    error_t err = cpymo_vars_set_cstr(vars, name_cstr, v);
    free(name_cstr);

    return err;
}

cpymo_val cpymo_vars_get(cpymo_vars *vars, cpymo_str name)
{
    const cpymo_val *p = cpymo_vars_access(vars, name);
    return p ? *p : 0;
}

error_t cpymo_vars_add(cpymo_vars *vars, cpymo_str name, cpymo_val v)
{
    error_t err = CPYMO_ERR_SUCC;
    char *name_cstr = cpymo_str_copy_malloc(name);
    if (name_cstr == NULL) return CPYMO_ERR_OUT_OF_MEM;

    cpymo_val *p = cpymo_vars_access_cstr(vars, name_cstr);

    if (p) *p += v;
    else err = cpymo_vars_set_cstr(vars, name_cstr, v);

    if (cpymo_vars_is_global(name_cstr))
        vars->globals_dirty = true;

    free(name_cstr);

    return err;
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

cpymo_val cpymo_vars_eval(cpymo_vars * vars, cpymo_str expr)
{
	if (cpymo_vars_is_constant(expr)) return cpymo_str_atoi(expr);
	else return cpymo_vars_get(vars, expr);
}

size_t cpymo_vars_count(void **var_field)
{
    struct cpymo_var **vars = (struct cpymo_var **)var_field;
    struct cpymo_var *hash_table = *vars;
    size_t sz = shlenu(hash_table);
    *vars = hash_table;
    return sz;
}

const char *cpymo_vars_get_by_index(void *var_field, size_t index, cpymo_val *v)
{
    const struct cpymo_var *var = index + (struct cpymo_var *)var_field;
    *v = var->value;
    return var->key;
}
