#include "cpymo_bg.h"
#include "cpymo_engine.h"
#include <stb_image.h>
#include <math.h>

void cpymo_bg_free(cpymo_bg *bg)
{
	if (bg->current_bg)
		cpymo_backend_image_free(bg->current_bg);
}

error_t cpymo_bg_update(cpymo_bg *bg, bool *redraw)
{
	if (bg->redraw) *redraw = true;

	bg->redraw = false;

	return CPYMO_ERR_SUCC;
}

void cpymo_bg_draw(cpymo_bg *bg)
{
	if (bg->current_bg) {
		cpymo_backend_image_draw(
			bg->current_bg_x,
			bg->current_bg_y,
			bg->current_bg_w,
			bg->current_bg_h,
			bg->current_bg,
			0,
			0,
			bg->current_bg_w,
			bg->current_bg_h,
			1.0f,
			cpymo_backend_image_draw_type_bg
		);
	}
}

error_t cpymo_bg_command(
	cpymo_engine *engine,
	cpymo_bg *bg,
	cpymo_parser_stream_span bgname,
	cpymo_parser_stream_span transition,
	float x,
	float y)
{
	if (bg->current_bg)
		cpymo_backend_image_free(bg->current_bg);
	bg->current_bg = NULL;

	char bg_name[40];
	cpymo_parser_stream_span_copy(bg_name, sizeof(bg_name), bgname);

	char *buf = NULL;
	size_t buf_size = 0;
	error_t err = cpymo_assetloader_load_bg(&buf, &buf_size, bg_name, &engine->assetloader);
	if (err != CPYMO_ERR_SUCC) return err;

	int w, h, channels;
	stbi_uc *pixels = stbi_load_from_memory((stbi_uc *)buf, buf_size, &w, &h, &channels, 3);
	free(buf);

	if (pixels == NULL)
		return CPYMO_ERR_UNKNOWN;

	err = cpymo_backend_image_load_immutable(
		&bg->current_bg,
		pixels,
		w,
		h,
		cpymo_backend_image_format_rgb);

	if (err != CPYMO_ERR_SUCC) {
		free(pixels);
		return err;
	}

	// In pymo, when x = y = 0 and bg smaller than screen, bg will be centered.
	if (fabs(x) < 1 && fabs(y) < 1 && w <= engine->gameconfig.imagesize_w && h <= engine->gameconfig.imagesize_h) {
		bg->current_bg_x = (float)(engine->gameconfig.imagesize_w - w) / 2.0f;
		bg->current_bg_y = (float)(engine->gameconfig.imagesize_h - h) / 2.0f;
	} 
	else {
		bg->current_bg_x = -(x / 100.0f) * (float)w;
		bg->current_bg_y = -(y / 100.0f) * (float)h;
	}

	bg->current_bg_w = w;
	bg->current_bg_h = h;

	bg->redraw = true;

	return CPYMO_ERR_SUCC;
}
