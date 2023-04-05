#include "../../cpymo/cpymo_prelude.h"
#include "../../cpymo/cpymo_utils.h"
#include "../include/cpymo_backend_movie.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

enum cpymo_backend_movie_how_to_play cpymo_backend_movie_how_to_play() {
	bool is_new_3ds = false;
	APT_CheckNew3DS(&is_new_3ds);

	return is_new_3ds ? 
		cpymo_backend_movie_how_to_play_send_surface :
		cpymo_backend_movie_how_to_play_unsupported;
}

int pad_tex_size(int s);

static C3D_Tex tex;
static Tex3DS_SubTexture subtex;
static C2D_Image image;
static size_t tex_edge_len;
static bool end_interrupt;
static C2D_DrawParams draw_params;

const extern bool drawing_bottom_screen;
static u16 *image_buf_line_by_line;

error_t cpymo_backend_movie_init_surface(size_t width, size_t height, enum cpymo_backend_movie_format format)
{
	if (width % 8) return CPYMO_ERR_UNSUPPORTED;
	Y2RU_InputFormat input_format;
	switch (format) {
	case cpymo_backend_movie_format_yuv420p: 
		input_format = INPUT_YUV420_INDIV_8;
		break;
	case cpymo_backend_movie_format_yuv420p16:
		input_format = INPUT_YUV420_INDIV_16;
		break;
	case cpymo_backend_movie_format_yuv422p:
		input_format = INPUT_YUV422_INDIV_8;
		break;
	case cpymo_backend_movie_format_yuv422p16:
		input_format = INPUT_YUV422_INDIV_16;
		break;
	case cpymo_backend_movie_format_yuyv422:
		input_format = INPUT_YUV422_BATCH;
		break;
	default: return CPYMO_ERR_UNSUPPORTED;
	};

	tex_edge_len = (size_t)pad_tex_size((int)(width > height ? width : height));
	if (tex_edge_len == -1) return CPYMO_ERR_UNSUPPORTED;

	if (!C3D_TexInit(&tex, tex_edge_len, tex_edge_len, GPU_RGB565)) {
		return CPYMO_ERR_OUT_OF_MEM;
	}

	C3D_TexSetFilter(&tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(&tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);

	subtex.width = (u16)width;
	subtex.height = (u16)height;
	subtex.left = 0;
	subtex.top = 0;
	subtex.right = (float)width / (float)tex_edge_len;
	subtex.bottom = (float)height / (float)tex_edge_len;

	image.tex = &tex;
	image.subtex = &subtex;

	if (R_FAILED(y2rInit())) {
		C3D_TexDelete(&tex);
		return CPYMO_ERR_OUT_OF_MEM;
	}

	Y2RU_ConversionParams p;
	p.input_format = input_format;
	p.output_format = OUTPUT_RGB_16_565;
	p.rotation = ROTATION_NONE;
	p.block_alignment = BLOCK_LINE;
	p.input_line_width = width;
	p.input_lines = height;
	p.standard_coefficient = COEFFICIENT_ITU_R_BT_709;
	p.alpha = 255;

	if (R_FAILED(Y2RU_SetConversionParams(&p))) {
		C3D_TexDelete(&tex);
		y2rExit();
	}

	end_interrupt = R_SUCCEEDED(Y2RU_SetTransferEndInterrupt(true));

	image_buf_line_by_line = (u16 *)linearAlloc(width * height * 2);
	if (image_buf_line_by_line == NULL) {
		C3D_TexDelete(&tex);
		y2rExit();
	}
	
	C3D_TexFlush(&tex);

	gfxSet3D(false);

	draw_params.angle = 0;
	draw_params.center.x = 0;
	draw_params.center.y = 0;
	draw_params.depth = 0;

	draw_params.pos.w = (float)width;
	draw_params.pos.h = (float)height;
	cpymo_utils_match_rect(400, 240, &draw_params.pos.w, &draw_params.pos.h);
	cpymo_utils_center(
		400, 240, 
		draw_params.pos.w, draw_params.pos.h,
		&draw_params.pos.x, &draw_params.pos.y);

	return CPYMO_ERR_SUCC;
}

void cpymo_backend_movie_free_surface()
{
	linearFree(image_buf_line_by_line);
	C3D_TexDelete(&tex);
	y2rExit();

	gfxSet3D(true);
}

static void cpymo_backend_movie_convert(u16 lines)
{
	u16 w;
	if (R_FAILED(Y2RU_GetInputLineWidth(&w))) return;
	if (R_FAILED(Y2RU_SetReceiving(
		image_buf_line_by_line, w * lines * 2, w * 2, 0))) return;

	if (R_FAILED(Y2RU_StartConversion())) return;

	bool done = false;
	do {
		if (end_interrupt) {
			Handle handle;
			if (R_SUCCEEDED(Y2RU_GetTransferEndEvent(&handle)))
				svcWaitSynchronization(handle, 1000000);
		}

		if (R_FAILED(Y2RU_IsDoneReceiving(&done))) {
			Y2RU_StopConversion();
			return;
		}
	} while (!done);

	for (size_t y = 0; y < lines; ++y) {
		for (size_t x = 0; x < w; ++x) {
			MAKE_PTR_TEX(o, tex, x, y, 2, tex_edge_len, tex_edge_len);
			*(u16 *)o = image_buf_line_by_line[y * w + x];
		}
	}

	C3D_TexFlush(&tex);
}

void cpymo_backend_movie_update_yuv_surface(
	const void *y, size_t y_pitch,
	const void *u, size_t u_pitch,
	const void *v, size_t v_pitch)
{
	u16 lines;
	if (R_FAILED(Y2RU_GetInputLines(&lines))) return;
	if (R_FAILED(Y2RU_SetSendingY(y, y_pitch * lines, y_pitch, 0))) return;
	if (R_FAILED(Y2RU_SetSendingU(u, u_pitch * lines, u_pitch, 0))) return;
	if (R_FAILED(Y2RU_SetSendingV(v, v_pitch * lines, v_pitch, 0))) return;

	cpymo_backend_movie_convert(lines);
}

void cpymo_backend_movie_update_yuyv_surface(const void *p, size_t pitch)
{
	u16 lines;
	if (R_FAILED(Y2RU_GetInputLines(&lines))) return;
	if (R_FAILED(Y2RU_SetSendingYUYV(p, pitch * lines, pitch, 0))) return;

	cpymo_backend_movie_convert(lines);
}

void cpymo_backend_movie_draw_surface()
{
	if (drawing_bottom_screen) return;

    C2D_ImageTint tint;
    C2D_AlphaImageTint(&tint, 1.0f);
	extern bool is_fill_screen(void);

	const static C2D_DrawParams full_screen_draw = {
		.angle = 0,
		.center.x = 0,
		.center.y = 0,
		.depth = 0,
		.pos.x = 0,
		.pos.y = 0,
		.pos.w = 400,
		.pos.h = 240
	};

    C2D_DrawImage(
		image, 
		is_fill_screen() ? &full_screen_draw : &draw_params, 
		&tint);
}

