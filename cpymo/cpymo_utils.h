#ifndef INCLUDE_CPYMO_UTILS
#define INCLUDE_CPYMO_UTILS

#include "cpymo_error.h"
#include <stddef.h>
#include <stdbool.h>

error_t cpymo_utils_loadfile(const char *path, char **outbuf, size_t *len);

struct cpymo_engine;
void *cpymo_utils_malloc_trim_memory(struct cpymo_engine *e, size_t size);

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

void cpymo_utils_attach_mask_to_rgba_ex(
	void *rgba, int w, int h, void *mask, int mask_w, int mask_h);

void cpymo_utils_match_rect(
	float container_w, float container_h, float *w, float *h);

void cpymo_utils_center(
	float container_w, float container_h, float w, float h, float *x, float *y);

static inline bool cpymo_utils_point_in_rect(
	float point_x, float point_y,
	float rect_x, float rect_y, float rect_w, float rect_h)
{
	return
		point_x >= rect_x &&
		point_x <= rect_x + rect_w &&
		point_y >= rect_y &&
		point_y <= rect_y + rect_h;
}

static inline bool cpymo_utils_point_in_rect_xywh(
	float point_x, float point_y, const float *rect_xywh)
{
	return cpymo_utils_point_in_rect(
		point_x, point_y, 
		rect_xywh[0],
		rect_xywh[1],
		rect_xywh[2],
		rect_xywh[3]);
}

#define CPYMO_ARR_COUNT(ARR) (sizeof(ARR) / sizeof(ARR[0]))

#define CPYMO_FOREACH_ARR(INDEX, ARR) \
	for (size_t INDEX = 0; INDEX < CPYMO_ARR_COUNT(ARR); ++INDEX)

#endif
