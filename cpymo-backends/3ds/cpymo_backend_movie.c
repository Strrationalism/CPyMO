#include <cpymo_prelude.h>
#include "cpymo_backend_movie.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
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
static size_t tex_edge_len;

const extern bool drawing_bottom_screen;
static void *fuck;		// TODO: Remove this!

error_t cpymo_backend_movie_init_surface(size_t width, size_t height, enum cpymo_backend_movie_format format)
{
	printf("[3DS Movie] Ready to init!\n");
	if (width % 8 || height % 8) return CPYMO_ERR_UNSUPPORTED;
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
	memset(tex.data, 0xFF, tex.size);

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
	p.standard_coefficient = COEFFICIENT_ITU_R_BT_601;
	p.alpha = 255;

	if (R_FAILED(Y2RU_SetConversionParams(&p))) {
		C3D_TexDelete(&tex);
		y2rExit();
	}

	fuck = linearAlloc(width * height * 2);
	
	C3D_TexFlush(&tex);

	printf("[3DS Movie] Init!\n");
	return CPYMO_ERR_SUCC;
}

void cpymo_backend_movie_free_surface()
{
	linearFree(fuck);
	printf("[3DS Movie] Ready to free!\n");
	C3D_TexDelete(&tex);
	y2rExit();
	printf("[3DS Movie] Free!\n");
}

static void cpymo_backend_movie_convert(u16 lines)
{
	u16 w;
	if (R_FAILED(Y2RU_GetInputLineWidth(&w))) return;
	if (R_FAILED(Y2RU_SetReceiving(
		fuck, w * lines * 2, w * 2, 0))) return;

	if (R_FAILED(Y2RU_StartConversion())) return;

	bool done = false;
	do {
		if (R_FAILED(Y2RU_IsDoneReceiving(&done))) {
			Y2RU_StopConversion();
			return;
		}
	} while (!done);

	for (size_t y = 0; y < lines; ++y) {
		for (size_t x = 0; x < w; ++x) {
			MAKE_PTR_TEX(o, tex, x, y, 2, tex_edge_len, tex_edge_len);
			*(u16 *)o = ((u16 *)fuck)[y * w + x];
		}
	}

	C3D_TexFlush(&tex);
}

void cpymo_backend_movie_update_yuv_surface(
	const void *y, size_t y_pitch,
	const void *u, size_t u_pitch,
	const void *v, size_t v_pitch)
{
	printf("[3DS Movie] Ready to set yuv surface!\n");
	u16 lines;
	if (R_FAILED(Y2RU_GetInputLines(&lines))) return;
	if (R_FAILED(Y2RU_SetSendingY(y, y_pitch * lines, y_pitch, 0))) return;
	if (R_FAILED(Y2RU_SetSendingU(u, u_pitch * lines, u_pitch, 0))) return;
	if (R_FAILED(Y2RU_SetSendingV(v, v_pitch * lines, v_pitch, 0))) return;

	cpymo_backend_movie_convert(lines);
	printf("[3DS Movie] Set yuv surface!\n");
}

void cpymo_backend_movie_update_yuyv_surface(const void *p, size_t pitch)
{
	printf("[3DS Movie] Ready to set yuyv surface!\n");
	u16 lines;
	if (R_FAILED(Y2RU_GetInputLines(&lines))) return;
	if (R_FAILED(Y2RU_SetSendingYUYV(p, pitch * lines, pitch, 0))) return;

	cpymo_backend_movie_convert(lines);
	printf("[3DS Movie] Set yuyv surface!\n");
}

void trans_size(float *w, float *h);
void trans_pos(float *x, float *y);
const extern float game_width, game_height;

void cpymo_backend_movie_draw_surface()
{
	printf("[3DS Movie] Ready to draw!\n");

	C2D_DrawParams p;
    p.angle = 0;
    p.center.x = 0;
    p.center.y = 0;
    p.depth = 0;

	p.pos.x = 0;
	p.pos.y = 0;
	p.pos.w = game_width;
	p.pos.h = game_height;

	trans_size(&p.pos.w, &p.pos.h);
	trans_pos(&p.pos.x, &p.pos.y);

    C2D_ImageTint tint;
    C2D_AlphaImageTint(&tint, 1.0f);

    C2D_DrawImage(image, &p, &tint);

	printf("[3DS Movie] Draw!\n");
}

