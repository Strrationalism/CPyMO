#include "cpymo_prelude.h"
#include "cpymo_str.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

cpymo_str cpymo_str_pure(const char *s)
{
	cpymo_str r;
	r.begin = s;
	r.len = strlen(s);
	return r;
}

void cpymo_str_trim_start(cpymo_str * span)
{
	size_t i;
	for (i = 0; i < span->len; ++i)
		if (span->begin[i] < 0 || !isblank(span->begin[i]))
			break;

	span->begin += i;
	span->len -= i;
}

void cpymo_str_trim_end(cpymo_str * span)
{
	if (span->len) {
		if (span->begin[span->len - 1] > 0 && isblank(span->begin[span->len - 1])) {
			span->len--;
			cpymo_str_trim_end(span);
		}
	}
}

void cpymo_str_copy(char *dst, size_t buffer_size, cpymo_str span)
{
	size_t copy_count = buffer_size - 1;
	if (span.len < copy_count) copy_count = span.len;

	strncpy(dst, span.begin, copy_count);
	dst[copy_count] = '\0';
}

char *cpymo_str_copy_malloc(cpymo_str str)
{
	char *cstr = (char *)malloc(str.len + 1);
	if (cstr == NULL) return NULL;

	memcpy(cstr, str.begin, str.len);
	cstr[str.len] = '\0';
	return cstr;
}

int cpymo_str_atoi(cpymo_str span)
{
	char buf[16];
	cpymo_str_trim(&span);
	cpymo_str_copy(buf, sizeof(buf), span);
	return atoi(buf);
}

float cpymo_str_atof(cpymo_str span)
{
	char buf[32];
	cpymo_str_trim(&span);
	cpymo_str_copy(buf, sizeof(buf), span);
	return (float)atof(buf);
}

void cpymo_str_trim(cpymo_str *span) 
{
	cpymo_str_trim_end(span);
	cpymo_str_trim_start(span);
}

static uint8_t from_hex_char(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	else return 0;
}

cpymo_color cpymo_str_as_color(cpymo_str span) 
{
	cpymo_str_trim(&span);

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

bool cpymo_str_equals_str(cpymo_str span, const char * str)
{
	return cpymo_str_equals(span, cpymo_str_pure(str));
}

bool cpymo_str_equals(cpymo_str a, cpymo_str b)
{
	if (a.len != b.len) return false;
	for (size_t i = 0; i < a.len; ++i)
		if (a.begin[i] != b.begin[i])
			return false;
	return true;
}

bool cpymo_str_equals_ignore_case(cpymo_str a, cpymo_str b)
{
	if (a.len != b.len) return false;
	for (size_t i = 0; i < a.len; ++i)
		if (tolower(a.begin[i]) != tolower(b.begin[i]))
			return false;
	return true;
}

bool cpymo_str_equals_str_ignore_case(cpymo_str a, const char * b)
{
	return cpymo_str_equals_ignore_case(a, cpymo_str_pure(b));
}

bool cpymo_str_starts_with_str_ignore_case(cpymo_str span, const char * prefix)
{
	if (*prefix == '\0') return true;
	else if (span.len == 0 && *prefix != '\0') return false;
	else if (toupper(span.begin[0]) == toupper(*prefix)) {
		span.len--;
		span.begin++;
		prefix++;
		return cpymo_str_starts_with_str_ignore_case(span, prefix);
	}
	else return false;
}

cpymo_str cpymo_str_utf8_try_head(cpymo_str *tail)
{
	if (tail->len == 0) return cpymo_str_pure("");
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

				cpymo_str span;
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

				cpymo_str span;
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
	return cpymo_str_pure("?");
}

uint32_t cpymo_str_utf8_try_head_to_utf32(cpymo_str *tail)
{
	cpymo_str ch = cpymo_str_utf8_try_head(tail);
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

size_t cpymo_str_utf8_len(cpymo_str span)
{
	size_t len = 0;
	while (span.len) {
		cpymo_str_utf8_try_head(&span);
		len++;
	}

	return len;
}

cpymo_str cpymo_str_split(cpymo_str *tail, size_t skip)
{
	cpymo_str span;
	span.begin = tail->begin;
	span.len = 0;
	for (size_t i = 0; i < skip && tail->len; ++i) {
		cpymo_str ch = cpymo_str_utf8_try_head(tail);
		span.len += ch.len;
	}

	return span;
}

void cpymo_str_hash_step(uint64_t *hash, char ch)
{
	const static uint64_t hash_seed = 131313;
	*hash = (*hash * hash_seed) + ch;
}

void cpymo_str_hash_append(
	uint64_t *hash, cpymo_str span)
{
	for (size_t i = 0; i < span.len; ++i)
		cpymo_str_hash_step(hash, span.begin[i]);
}

void cpymo_str_hash_append_cstr(uint64_t *hash, const char *s)
{
	char ch;
	while ('\0' != (ch = *s++))
		cpymo_str_hash_step(hash, ch);
}

