#ifndef INCLUDE_cpymo_str
#define INCLUDE_cpymo_str

#include <stddef.h>
#include <stdbool.h>
#include "cpymo_color.h"

typedef struct {
	const char *begin;
	size_t len;
} cpymo_str;

cpymo_str cpymo_str_pure(const char *);

void cpymo_str_trim_start(cpymo_str *span);
void cpymo_str_trim_end(cpymo_str *span);
void cpymo_str_trim(cpymo_str *span);
void cpymo_str_copy(char *dst, size_t buffer_size, cpymo_str span);
char *cpymo_str_copy_malloc(cpymo_str str);
int cpymo_str_atoi(cpymo_str span);
float cpymo_str_atof(cpymo_str span);
cpymo_color cpymo_str_as_color(cpymo_str span);

bool cpymo_str_equals_str(cpymo_str span, const char *str);
bool cpymo_str_equals(cpymo_str a, cpymo_str b);
bool cpymo_str_equals_ignore_case(cpymo_str a, cpymo_str b);
bool cpymo_str_equals_str_ignore_case(cpymo_str a, const char *b);
bool cpymo_str_starts_with_str_ignore_case(cpymo_str span, const char *prefix);

cpymo_str cpymo_str_utf8_try_head(cpymo_str *tail);
uint32_t cpymo_str_utf8_try_head_to_utf32(cpymo_str *tail);
size_t cpymo_str_utf8_len(cpymo_str span);
cpymo_str cpymo_str_split(cpymo_str *tail, size_t skip);

static inline void cpymo_str_hash_init(uint64_t *hash) 
{ *hash = 0; }

void cpymo_str_hash_step(uint64_t *hash, char ch);
void cpymo_str_hash_append_cstr(uint64_t *hash, const char *s);
void cpymo_str_hash_append(
	uint64_t *hash, cpymo_str span);

#endif
