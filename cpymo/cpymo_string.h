#ifndef INCLUDE_CPYMO_STRING
#define INCLUDE_CPYMO_STRING

#include <stddef.h>
#include <stdbool.h>
#include "cpymo_color.h"

typedef struct {
	const char *begin;
	size_t len;
} cpymo_string;

cpymo_string cpymo_string_pure(const char *);

void cpymo_string_trim_start(cpymo_string *span);
void cpymo_string_trim_end(cpymo_string *span);
void cpymo_string_trim(cpymo_string *span);
void cpymo_string_copy(char *dst, size_t buffer_size, cpymo_string span);
int cpymo_string_atoi(cpymo_string span);
float cpymo_string_atof(cpymo_string span);
cpymo_color cpymo_string_as_color(cpymo_string span);

bool cpymo_string_equals_str(cpymo_string span, const char *str);
bool cpymo_string_equals(cpymo_string a, cpymo_string b);
bool cpymo_string_equals_ignore_case(cpymo_string a, cpymo_string b);
bool cpymo_string_equals_str_ignore_case(cpymo_string a, const char *b);
bool cpymo_string_starts_with_str_ignore_case(cpymo_string span, const char *prefix);

cpymo_string cpymo_string_utf8_try_head(cpymo_string *tail);
uint32_t cpymo_string_utf8_try_head_to_utf32(cpymo_string *tail);
size_t cpymo_string_utf8_len(cpymo_string span);
cpymo_string cpymo_string_split(cpymo_string *tail, size_t skip);

static inline void cpymo_string_hash_init(uint64_t *hash) 
{ *hash = 0; }

void cpymo_string_hash_step(uint64_t *hash, char ch);
void cpymo_string_hash_append_cstr(uint64_t *hash, const char *s);
void cpymo_string_hash_append(
	uint64_t *hash, cpymo_string span);

#endif
