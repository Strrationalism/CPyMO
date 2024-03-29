﻿#include "cpymo_prelude.h"
#include "cpymo_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

error_t cpymo_utils_loadfile(const char *path, char **outbuf, size_t *len)
{
	assert(*outbuf == NULL);

	FILE *f = fopen(path, "rb");
	if (f == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	fseek(f, 0, SEEK_END);
	*len = ftell(f);
	fseek(f, 0, SEEK_SET);

	*outbuf = (char *)malloc(*len);
	if (*outbuf == NULL) {
		fclose(f);
		return CPYMO_ERR_OUT_OF_MEM;
	}

	if (fread(*outbuf, *len, 1, f) != 1) {
		fclose(f);
		free(*outbuf);
		return CPYMO_ERR_CAN_NOT_OPEN_FILE;
	}

	fclose(f);
	return CPYMO_ERR_SUCC;
}

#ifndef CPYMO_TOOL
void *cpymo_utils_malloc_trim_memory(struct cpymo_engine *e, size_t size)
{
	void *ret = malloc(size);
	if (ret == NULL && e) {
		extern void cpymo_engine_trim_memory(struct cpymo_engine *);
		cpymo_engine_trim_memory(e);
		ret = malloc(size);
	}

	return ret;
}
#endif

float cpymo_utils_lerp(float a, float b, float x)
{
	float t = (float)(x < 0.5 ? 2 * x * x : 1 - pow(-2 * x + 2, 2) / 2);
	return (b - a) * t + a;
}

void cpymo_utils_replace_str_newline_n(char *r)
{
	char *w = r;

	bool got_leader = false;
	while (*r) {
		if (got_leader) {
			if (*r == 'n' || *r == 'r') *(w++) = '\n';
			else {
				*(w++) = '\\';
				*(w++) = *r;
			}

			got_leader = false;
		}
		else {
			if (*r == '\\') got_leader = true;
			else *(w++) = *r;
		}

		r++;
	}

	*w = '\0';
}

void cpymo_utils_replace_cr(char *text, size_t len)
{
	for (size_t i = 0; i < len; ++i) {
		if (text[i] == '\r') {
			if (i + 1 < len) {
				if (text[i + 1] != '\n') { text[i] = '\n'; }
			}
			else {
				text[i] = '\n';
			}
		}
	}
}

void cpymo_utils_attach_mask_to_rgba(void *rgba_, void *mask_, int w, int h)
{
	char *rgba = (char *)rgba_;
	char *mask = (char *)mask_;

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			rgba[(y * w + x) * 4 + 3] = mask[y * w + x];
		}
	}
}

void cpymo_utils_attach_mask_to_rgba_ex(void * rgba_, int w, int h, void * mask_, int mask_w, int mask_h)
{
	if (w == mask_w && h == mask_h) {
		cpymo_utils_attach_mask_to_rgba(rgba_, mask_, w, h);
		return;
	}
	
	char *rgba = (char *)rgba_;
	char *mask = (char *)mask_;

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			float fx = (float)x / (float)w;
			float fy = (float)y / (float)h;

			int mx = (int)(fx * (float)mask_w);
			int my = (int)(fy * (float)mask_h);
			rgba[(y * w + x) * 4 + 3] = mask[my * mask_w + mx];
		}
	}
}

void cpymo_utils_match_rect(
	float container_w, float container_h, float *w, float *h)
{
	float ratio_w = *w / container_w;
	float ratio_h = *h / container_h;
	float ratio = ratio_w > ratio_h ? ratio_w : ratio_h;
	*w /= ratio;
	*h /= ratio;
}

void cpymo_utils_center(
	float container_w, float container_h, float w, float h, float *x, float *y)
{
	*x += (container_w - w) / 2;
	*y += (container_h - h) / 2;
}

#ifndef CPYMO_TOOL
#include "cpymo_engine.h"
enum cpymo_key_hold_result cpymo_key_hold_update(
	cpymo_engine *e, cpymo_key_hold *h, float dt, bool pressed)
{
	const float 
		cpymo_key_hold_time = 0.4f,
		cpymo_key_hold_cancel_speed = 0.7f;

	if (!pressed) {
		h->ignore_current = false;
		h->hold_canceled = false;
	}

	if (h->ignore_current) return cpymo_key_hold_result_released;

	const bool prev_pressed = h->prev_pressed;
	h->prev_pressed = pressed;

	if (!pressed && !prev_pressed) {
		h->timer = 0;
		return cpymo_key_hold_result_released;
	}
	else if (pressed && !prev_pressed) {
		h->timer = 0;
		return cpymo_key_hold_result_just_press;
	}
	else if (pressed && prev_pressed) {
		float mouse_speed = 0;
		if (e->input.mouse_position_useable && e->input.mouse_position_useable) {
			float 
				mouse_distance_x = fabsf(e->input.mouse_x - e->prev_input.mouse_x),
				mouse_distance_y = fabsf(e->input.mouse_y - e->prev_input.mouse_y);
			float mouse_distance = mouse_distance_x;
			if (mouse_distance_y > mouse_distance) 
				mouse_distance = mouse_distance_y;
			mouse_speed = mouse_distance / (dt > 0.00001f ? dt : 0.016f);
			float max_screen_edge = e->gameconfig.imagesize_h;
			if (max_screen_edge < e->gameconfig.imagesize_w) 
				max_screen_edge = e->gameconfig.imagesize_w;
			mouse_speed /= max_screen_edge;
		}

		if (mouse_speed > cpymo_key_hold_cancel_speed) {
			if (!h->hold_canceled) {
				h->hold_canceled = true;
				if (h->timer >= cpymo_key_hold_time)
					return cpymo_key_hold_result_cancel;
			}
		}
	
		if (h->hold_canceled) return cpymo_key_hold_result_pressing;
		h->timer += dt;
		if (h->timer - dt < cpymo_key_hold_time && h->timer >= cpymo_key_hold_time) 
			return cpymo_key_hold_result_just_hold;
		else if (h->timer >= cpymo_key_hold_time) 
			return cpymo_key_hold_result_holding;
		else return cpymo_key_hold_result_pressing;
	}
	else {//if (!pressed && prev_pressed) {
		if (h->timer >= cpymo_key_hold_time && !h->hold_canceled) 
			return cpymo_key_hold_result_hold_released;
		else return cpymo_key_hold_result_just_released;
	}
}
#endif