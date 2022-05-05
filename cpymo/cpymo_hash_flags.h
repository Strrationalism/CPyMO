#ifndef INCLUDE_CPYMO_HASH_FLAGS
#define INCLUDE_CPYMO_HASH_FLAGS

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "cpymo_error.h"

typedef uint64_t cpymo_hash_flag;

typedef struct {
	cpymo_hash_flag *flags;
	size_t flag_count;
	size_t flag_buf_size;
	bool dirty;
} cpymo_hash_flags;

void cpymo_hash_flags_init(cpymo_hash_flags *);
void cpymo_hash_flags_free(cpymo_hash_flags *);

error_t cpymo_hash_flags_reserve(cpymo_hash_flags *, size_t);
error_t cpymo_hash_flags_add(cpymo_hash_flags *, cpymo_hash_flag);
bool cpymo_hash_flags_check(cpymo_hash_flags *, cpymo_hash_flag);

#endif