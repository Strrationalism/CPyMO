#include "cpymo_backend_movie.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

enum cpymo_backend_movie_how_to_play cpymo_backend_movie_how_to_play() {
	return cpymo_backend_movie_how_to_play_send_surface;
}

int pad_tex_size(int s);

static C3D_Tex tex;
static Tex3DS_SubTexture subtex;
static C2D_Image image;
static u8 *tex_line_by_line = NULL;

error_t cpymo_backend_movie_init_surface(size_t width, size_t height, enum cpymo_backend_movie_format format)
{
	assert(tex_line_by_line == NULL);
	int pad_width = pad_tex_size(width);
	int pad_height = pad_tex_size(height);

	if (pad_width == -1 || pad_height == -1) return CPYMO_ERR_UNSUPPORTED;

	int pad_edge = pad_width > pad_height ? pad_width : pad_height;

	if (R_FAILED(y2rInit())) return CPYMO_ERR_UNKNOWN;

	Y2RU_ConversionParams params;

	switch (format) {
		case cpymo_backend_movie_format_yuv420p:
			params.input_format = INPUT_YUV420_INDIV_8;
			break;
		case cpymo_backend_movie_format_yuv422p:
			params.input_format = INPUT_YUV422_INDIV_8;
			break;
		case cpymo_backend_movie_format_yuv420p16:
			params.input_format = INPUT_YUV420_INDIV_16;
			break;
		case cpymo_backend_movie_format_yuv422p16:
			params.input_format = INPUT_YUV422_INDIV_16;
			break;
		case cpymo_backend_movie_format_yuyv422:
			params.input_format = INPUT_YUV422_BATCH;
			break;
		default:
			y2rExit();
			return CPYMO_ERR_UNSUPPORTED;
	}

	params.output_format = OUTPUT_RGB_16_565;
	params.rotation = ROTATION_NONE;
	params.block_alignment = BLOCK_LINE;
	params.alpha = 0xFF;
	params.input_line_width = (s16)width;
	params.input_lines = (s16)height;
	params.standard_coefficient = COEFFICIENT_ITU_R_BT_709_SCALING;

	if (R_FAILED(Y2RU_SetConversionParams(&params))) 
	{ y2rExit(); return CPYMO_ERR_UNKNOWN; }

	if (!C3D_TexInit(&tex, (u16)pad_edge, (u16)pad_edge, GPU_RGB565)) {
		y2rExit(); return CPYMO_ERR_OUT_OF_MEM;
	}

	C3D_TexSetFilter(&tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(&tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);

	subtex.width = (u16)width;
	subtex.height = (u16)height;
	subtex.left = 0;
	subtex.top = 0;
	subtex.right = (float)width / (float)pad_edge;
	subtex.bottom = (float)height / (float)pad_edge;

	image.tex = &tex;
	image.subtex = &subtex;

	tex_line_by_line = malloc(width * height * 2);
	if (tex_line_by_line == NULL) 
	{ 
		C3D_TexDelete(&tex);
		y2rExit(); 
		return CPYMO_ERR_OUT_OF_MEM; 
	}
	
	return CPYMO_ERR_SUCC;
}

void cpymo_backend_movie_free_surface()
{
	assert(tex_line_by_line != NULL);
	y2rExit();
	C3D_TexDelete(&tex);
	free(tex_line_by_line);
}

static void cpymo_backend_movie_update_tex()
{
	Result r = Y2RU_SetReceiving(
		tex_line_by_line, 
		tex.width * tex.height * 2, 
		tex.width * 2, 
		0);
	if (R_FAILED(r)) return;


	Y2RU_StartConversion();

	bool finished = false;
	while (!finished) {
		if (R_FAILED(Y2RU_IsDoneReceiving(&finished))) return;
	}

	if (tex_line_by_line) {
		for (size_t y = 0; y < subtex.height; ++y) {
			for (size_t x = 0; x < subtex.width; ++x) {
				MAKE_PTR_TEX(out, tex, x, y, 2, tex.width, tex.height);
				out[0] = tex_line_by_line[(y * subtex.width + x) * 2];
				out[1] = tex_line_by_line[(y * subtex.width + x) * 2 + 1];
			}
		}
	}

	C3D_TexFlush(&tex);
}

void cpymo_backend_movie_update_yuv_surface(
	const void *y, size_t y_pitch,
	const void *u, size_t u_pitch,
	const void *v, size_t v_pitch)
{
	Result r = Y2RU_SetSendingY(
		y,
		(u32)subtex.width * (u32)subtex.height, 
		y_pitch,
		0);
	if (R_FAILED(r)) return;

	r = Y2RU_SetSendingU(
		u,
		(u32)subtex.width * (u32)subtex.height / ((y_pitch / u_pitch) * (y_pitch / u_pitch)), 
		u_pitch,
		0);
	if (R_FAILED(r)) return;

	r = Y2RU_SetSendingV(
		v,
		(u32)subtex.width * (u32)subtex.height / ((y_pitch / v_pitch) * (y_pitch / v_pitch)),
		v_pitch,
		0);
	if (R_FAILED(r)) return;

	cpymo_backend_movie_update_tex();
}

void cpymo_backend_movie_update_yuyv_surface(const void *p, size_t pitch)
{
	Result r = Y2RU_SetSendingYUYV(
		p,
		(u32)subtex.width * (u32)subtex.height, 
		pitch,
		0);
	if (R_FAILED(r)) return;

	cpymo_backend_movie_update_tex();
}

void cpymo_backend_movie_draw_surface()
{
	C2D_DrawParams p;
    p.angle = 0;
    p.center.x = 0;
    p.center.y = 0;
    p.depth = 0;
    p.pos.x = 0;
    p.pos.y = 0;
    p.pos.w = 400;
    p.pos.h = 240;

    C2D_ImageTint tint;
    C2D_AlphaImageTint(&tint, 1.0f);

    C2D_DrawImage(image, &p, &tint);
}
