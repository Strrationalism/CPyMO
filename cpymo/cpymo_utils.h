#ifndef INCLUDE_CPYMO_UTILS
#define INCLUDE_CPYMO_UTILS

#include "cpymo_error.h"
#include <stddef.h>
#include <stdbool.h>

error_t cpymo_utils_loadfile(const char *path, char **outbuf, size_t *len);

static inline int cpymo_utils_clamp(int v, int minv, int maxv)
{
	if (v > maxv) return maxv;
	else if (v < minv) return minv;
	else return v;
}

static inline float cpymo_utils_clampf(float v, float minv, float maxv)
{
	if (v > maxv) return maxv;
	else if (v < minv) return minv;
	else return v;
}

float cpymo_utils_lerp(float a, float b, float t);

void cpymo_utils_replace_str_newline_n(char *str);

void cpymo_utils_replace_cr(char *text, size_t len);

void cpymo_utils_attach_mask_to_rgba(void *rgba, void *mask, int w, int h);

void cpymo_utils_attach_mask_to_rgba_ex(void *rgba, int w, int h, void *mask, int mask_w, int mask_h);

#endif
