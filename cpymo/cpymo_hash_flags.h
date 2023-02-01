#ifndef INCLUDE_CPYMO_HASH_FLAGS
#define INCLUDE_CPYMO_HASH_FLAGS

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "cpymo_error.h"

typedef uint64_t cpymo_hash_flag;

typedef struct {
	void *flags;
	bool dirty;
} cpymo_hash_flags;

void cpymo_hash_flags_init(cpymo_hash_flags *);
void cpymo_hash_flags_free(cpymo_hash_flags *);

error_t cpymo_hash_flags_add(cpymo_hash_flags *, cpymo_hash_flag);
void cpymo_hash_flags_del(cpymo_hash_flags *, cpymo_hash_flag);
bool cpymo_hash_flags_check(cpymo_hash_flags *, cpymo_hash_flag);

size_t cpymo_hash_flags_count(cpymo_hash_flags *);
uint64_t cpymo_hash_flags_get(cpymo_hash_flags *, size_t index);

#endif