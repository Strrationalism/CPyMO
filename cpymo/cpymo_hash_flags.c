#include "cpymo_prelude.h"
#include "cpymo_hash_flags.h"
#include <stdlib.h>
#include <assert.h>
#include <stb_ds.h>

typedef struct {
	cpymo_hash_flag key;
} cpymo_hash_flags_internal;

void cpymo_hash_flags_init(cpymo_hash_flags *f)
{
	f->flags = NULL;
	f->dirty = false;
}

void cpymo_hash_flags_free(cpymo_hash_flags *f)
{
	cpymo_hash_flags_internal *flags = (cpymo_hash_flags_internal *)f->flags;
	if (flags) hmfree(flags);
}

error_t cpymo_hash_flags_add(cpymo_hash_flags *fs, cpymo_hash_flag f)
{
	if (cpymo_hash_flags_check(fs, f)) return CPYMO_ERR_SUCC;

	cpymo_hash_flags_internal *flags = (cpymo_hash_flags_internal *)fs->flags;
	cpymo_hash_flags_internal kv;
	kv.key = f;
	hmputs(flags, kv);
	fs->flags = (void *)flags;
	fs->dirty = true;
	return CPYMO_ERR_SUCC;
}

bool cpymo_hash_flags_check(cpymo_hash_flags *fs, cpymo_hash_flag f)
{
	cpymo_hash_flags_internal *flags = (cpymo_hash_flags_internal *)fs->flags;

	cpymo_hash_flags_internal *ret = hmgetp_null(flags, f);
	fs->flags = (void *)flags;

	if (ret) 
		return true;
	else return false;
}

size_t cpymo_hash_flags_count(cpymo_hash_flags *fs)
{
	cpymo_hash_flags_internal *f = (cpymo_hash_flags_internal *)fs->flags;
	size_t len = hmlenu(f);
	fs->flags = (void *)f;

	return len;
}

uint64_t cpymo_hash_flags_get(cpymo_hash_flags *fs, size_t index)
{
	cpymo_hash_flags_internal *f = (cpymo_hash_flags_internal *)fs->flags;
	return f[index].key;
}