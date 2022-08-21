#include "cpymo_prelude.h"
#include "cpymo_string.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

cpymo_string cpymo_string_pure(const char *s)
{
	cpymo_string r;
	r.begin = s;
	r.len = strlen(s);
	return r;
}

void cpymo_string_trim_start(cpymo_string * span)
{
	size_t i;
	for (i = 0; i < span->len; ++i)
		if (span->begin[i] < 0 || !isblank(span->begin[i]))
			break;

	span->begin += i;
	span->len -= i;
}

void cpymo_string_trim_end(cpymo_string * span)
{
	if (span->len) {
		if (span->begin[span->len - 1] > 0 && isblank(span->begin[span->len - 1])) {
			span->len--;
			cpymo_string_trim_end(span);
		}
	}
}

void cpymo_string_copy(char *dst, size_t buffer_size, cpymo_string span)
{
	size_t copy_count = buffer_size - 1;
	if (span.len < copy_count) copy_count = span.len;

	strncpy(dst, span.begin, copy_count);
	dst[copy_count] = '\0';
}

int cpymo_string_atoi(cpymo_string span)
{
	char buf[16];
	cpymo_string_trim(&span);
	cpymo_string_copy(buf, sizeof(buf), span);
	return atoi(buf);
}

float cpymo_string_atof(cpymo_string span)
{
	char buf[32];
	cpymo_string_trim(&span);
	cpymo_string_copy(buf, sizeof(buf), span);
	return (float)atof(buf);
}

void cpymo_string_trim(cpymo_string *span) 
{
	cpymo_string_trim_end(span);
	cpymo_string_trim_start(span);
}

static uint8_t from_hex_char(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	else return 0;
}

cpymo_color cpymo_string_as_color(cpymo_string span) 
{
	cpymo_string_trim(&span);

	if (span.begin[0] != '#') return cpymo_color_error;
	if (span.len < 1) return cpymo_color_error;

	for (size_t i = 0; i < span.len - 1; ++i) 
		if (span.begin[i + 1] < 0 || !isxdigit((int)span.begin[i + 1]))
			return cpymo_color_error;
	
	cpymo_color c;
	c.r = 0;
	c.g = 0;
	c.b = 0;

	size_t i = span.len - 1;

	if (i == 0) return c;

	c.b += from_hex_char(span.begin[i--]);
	if (i == 0) return c;

	c.b += from_hex_char(span.begin[i--]) * 16;
	if (i == 0) return c;

	c.g += from_hex_char(span.begin[i--]);
	if (i == 0) return c;

	c.g += from_hex_char(span.begin[i--]) * 16;
	if (i == 0) return c;

	c.r += from_hex_char(span.begin[i--]);
	if (i == 0) return c;

	c.r += from_hex_char(span.begin[i--]) * 16;
	if (i == 0) return c;

	return c;
}

bool cpymo_string_equals_str(cpymo_string span, const char * str)
{
	return cpymo_string_equals(span, cpymo_string_pure(str));
}

bool cpymo_string_equals(cpymo_string a, cpymo_string b)
{
	if (a.len != b.len) return false;
	for (size_t i = 0; i < a.len; ++i)
		if (a.begin[i] != b.begin[i])
			return false;
	return true;
}

bool cpymo_string_equals_ignore_case(cpymo_string a, cpymo_string b)
{
	if (a.len != b.len) return false;
	for (size_t i = 0; i < a.len; ++i)
		if (tolower(a.begin[i]) != tolower(b.begin[i]))
			return false;
	return true;
}

bool cpymo_string_equals_str_ignore_case(cpymo_string a, const char * b)
{
	return cpymo_string_equals_ignore_case(a, cpymo_string_pure(b));
}

bool cpymo_string_starts_with_str_ignore_case(cpymo_string span, const char * prefix)
{
	if (*prefix == '\0') return true;
	else if (span.len == 0 && *prefix != '\0') return false;
	else if (toupper(span.begin[0]) == toupper(*prefix)) {
		span.len--;
		span.begin++;
		prefix++;
		return cpymo_string_starts_with_str_ignore_case(span, prefix);
	}
	else return false;
}

cpymo_string cpymo_string_utf8_try_head(cpymo_string *tail)
{
	if (tail->len == 0) return cpymo_string_pure("");
	else {
		unsigned head_ones = 0;
		unsigned char c = (unsigned char)tail->begin[0];
		while (c & 0x80) {
			head_ones++;
			c = c << 1;
		}

		if (head_ones == 0 || (head_ones >= 2 && head_ones <= 6)) {
			if (head_ones == 0) {
				tail->begin++;
				tail->len--;

				cpymo_string span;
				span.begin = tail->begin - 1;
				span.len = 1;
				return span;
			}
			else {
				unsigned bytes = head_ones;
				unsigned following_bytes = bytes - 1;

				if (tail->len < bytes)
					goto BAD_UTF8;
				
				for (unsigned i = 0; i < following_bytes; ++i) {
					unsigned following_byte = tail->begin[1 + i];

					if ((following_byte & 0xC0) != 0x80)
						goto BAD_UTF8;
				}

				cpymo_string span;
				span.begin = tail->begin;
				span.len = bytes;

				tail->begin += bytes;
				tail->len -= bytes;
				return span;
			}
		}
		else goto BAD_UTF8;
	}

BAD_UTF8:
	tail->begin++;
	tail->len--;
	return cpymo_string_pure("?");
}

uint32_t cpymo_string_utf8_try_head_to_utf32(cpymo_string *tail)
{
	cpymo_string ch = cpymo_string_utf8_try_head(tail);
	uint32_t result = 0;

	for (size_t i = 0; i < ch.len; ++i) {
		result <<= 6;
		unsigned char byte = (unsigned char)ch.begin[i];
		unsigned char mask = 0x80;

		while ((byte & mask) > 0) {
			byte &= ~mask;
			mask >>= 1;
		}

		result |= byte;
	}

	return result;
}

size_t cpymo_string_utf8_len(cpymo_string span)
{
	size_t len = 0;
	while (span.len) {
		cpymo_string_utf8_try_head(&span);
		len++;
	}

	return len;
}

cpymo_string cpymo_string_split(cpymo_string *tail, size_t skip)
{
	cpymo_string span;
	span.begin = tail->begin;
	span.len = 0;
	for (size_t i = 0; i < skip && tail->len; ++i) {
		cpymo_string ch = cpymo_string_utf8_try_head(tail);
		span.len += ch.len;
	}

	return span;
}

void cpymo_string_hash_step(uint64_t *hash, char ch)
{
	const static uint64_t hash_seed = 131313;
	*hash = (*hash * hash_seed) + ch;
}

void cpymo_string_hash_append(
	uint64_t *hash, cpymo_string span)
{
	for (size_t i = 0; i < span.len; ++i)
		cpymo_string_hash_step(hash, span.begin[i]);
}

void cpymo_string_hash_append_cstr(uint64_t *hash, const char *s)
{
	char ch;
	while ('\0' != (ch = *s++))
		cpymo_string_hash_step(hash, ch);
}

