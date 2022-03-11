#include "cpymo_backend_movie.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <libswscale/swscale.h>

enum cpymo_backend_movie_how_to_play cpymo_backend_movie_how_to_play() {
	return cpymo_backend_movie_how_to_play_send_surface;
}

int pad_tex_size(int s);

static C3D_Tex tex;
static Tex3DS_SubTexture subtex;
static C2D_Image image;
static u8 *tex_line_by_line = NULL;
const extern bool fill_screen;
static struct SwsContext *sws = NULL;
static size_t origin_height;
const extern bool drawing_bottom_screen;

error_t cpymo_backend_movie_init_surface(size_t width, size_t height, enum cpymo_backend_movie_format format)
{
	assert(tex_line_by_line == NULL);
	origin_height = height;

	enum AVPixelFormat fmt;
	switch (format) {
		case cpymo_backend_movie_format_yuv420p:
			fmt = AV_PIX_FMT_YUV420P;
			break;
		case cpymo_backend_movie_format_yuv422p:
			fmt = AV_PIX_FMT_YUV422P;
			break;
		case cpymo_backend_movie_format_yuv420p16:
			fmt = AV_PIX_FMT_YUV420P16;
			break;
		case cpymo_backend_movie_format_yuv422p16:
			fmt = AV_PIX_FMT_YUV422P16;
			break;
		case cpymo_backend_movie_format_yuyv422:
			fmt = AV_PIX_FMT_YUYV422;
			break;
		default: return CPYMO_ERR_UNSUPPORTED;
	}

	if (!C3D_TexInit(&tex, 512, 512, GPU_RGB565)) {
		return CPYMO_ERR_OUT_OF_MEM;
	}

	C3D_TexSetFilter(&tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(&tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);

	float ratio_w = (float)width / 400.0f;
	float ratio_h = (float)height / 240.0f;
	float ratio = ratio_w > ratio_h ? ratio_w : ratio_h;
	float target_w = width / ratio;
	float target_h = height / ratio;

	subtex.width = (u16)target_w;
	subtex.height = (u16)target_h;
	subtex.left = 0;
	subtex.top = 0;
	subtex.right = (float)subtex.width / 512.0f;
	subtex.bottom = (float)subtex.height / 512.0f;

	image.tex = &tex;
	image.subtex = &subtex;

	tex_line_by_line = malloc(subtex.width * subtex.height * 2);
	if (tex_line_by_line == NULL) 
	{ 
		C3D_TexDelete(&tex);
		return CPYMO_ERR_OUT_OF_MEM; 
	}

	sws = sws_getContext(
		(int)width,
		(int)height,
		fmt,
		(int)subtex.width,
		(int)subtex.height,
		AV_PIX_FMT_RGB565,
		SWS_FAST_BILINEAR,
		NULL,
		NULL,
		NULL
	);

	if (sws == NULL) {
		free(tex_line_by_line);
		C3D_TexDelete(&tex);
		return CPYMO_ERR_OUT_OF_MEM;
	}
	
	return CPYMO_ERR_SUCC;
}

void cpymo_backend_movie_free_surface()
{
	assert(tex_line_by_line != NULL);
	sws_freeContext(sws);
	C3D_TexDelete(&tex);
	free(tex_line_by_line);
	tex_line_by_line = NULL;
}

static void cpymo_backend_movie_update_surface()
{
	for (size_t y = 0; y < subtex.height; y++) {
		for (size_t x = 0; x < subtex.width; ++x) {
			MAKE_PTR_TEX(out, tex, x, y, 2, tex.width, tex.height);
			out[0] = tex_line_by_line[(y * subtex.width + x) * 2];
			out[1] = tex_line_by_line[(y * subtex.width + x) * 2 + 1];
		}
	}

	C3D_TexFlush(&tex);
}

void cpymo_backend_movie_update_yuv_surface(
	const void *y, size_t y_pitch,
	const void *u, size_t u_pitch,
	const void *v, size_t v_pitch)
{
	const uint8_t *px[] = {
		(const uint8_t *)y,
		(const uint8_t *)u,
		(const uint8_t *)v
	};

	const int px_pitch[] = {
		(int)y_pitch,
		(int)u_pitch,
		(int)v_pitch
	};

	uint8_t *dst[] = { (uint8_t *)tex_line_by_line	};
	const int dst_pitch[] = { (int)subtex.width * 2 };

	sws_scale(sws, px, px_pitch, 0, (int)origin_height, dst, dst_pitch);
	cpymo_backend_movie_update_surface();
}

void cpymo_backend_movie_update_yuyv_surface(const void *p, size_t pitch)
{
}

void cpymo_backend_movie_draw_surface()
{
	if (drawing_bottom_screen) return;
	C2D_DrawParams p;
    p.angle = 0;
    p.center.x = 0;
    p.center.y = 0;
    p.depth = 0;

	if (fill_screen) {
		p.pos.x = 0;
		p.pos.y = 0;
		p.pos.w = 400;
		p.pos.h = 240;
	}
	else {
		p.pos.x = (400 - subtex.width) / 2;
		p.pos.y = (240 - subtex.height) / 2;
		p.pos.w = subtex.width;
		p.pos.h = subtex.height;
	}

    C2D_ImageTint tint;
    C2D_AlphaImageTint(&tint, 1.0f);

    C2D_DrawImage(image, &p, &tint);
}
