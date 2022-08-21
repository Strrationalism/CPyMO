#include "cpymo_prelude.h"
#include <cpymo_backend_image.h>
#include <cpymo_backend_masktrans.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cpymo_utils.h>
#include <cpymo_engine.h>
#include <stb_image_resize.h>

#ifdef _WIN32

#include <windows.h>
static void get_winsize(size_t *w, size_t *h) 
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    *w = (size_t)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    *h = (size_t)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);

    if (*w <= 5) *w = 80;
    if (*h <= 5) *h = 25;
}

#else

#include <sys/ioctl.h>
#include <unistd.h>
static void get_winsize(size_t *w, size_t *h)
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *w = (size_t)ws.ws_col;
    *h = (size_t)ws.ws_row;

    if (*w <= 1) *w = 80;
    if (*h <= 1) *h = 25;
}

#endif

extern cpymo_engine engine;
size_t window_size_w = 80, window_size_h = 25;

const static char ascii_table[71] = 
	" .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8B@$";

const static size_t ascii_table_length = 69;

static void put_pixel_rgb(uint8_t r, uint8_t g, uint8_t b);

static void (* const put_pixel)(uint8_t r, uint8_t g, uint8_t b) = &put_pixel_rgb;

uint8_t *framebuffer = NULL;
static char *framebuffer_ascii = NULL;
static size_t framebuffer_ascii_cur_size = 0;
static size_t framebuffer_ascii_cur = 0;

static error_t cpymo_backend_image_write_string(const char *str) 
{
	size_t str_len = strlen(str);
	if (framebuffer_ascii_cur + str_len + 1 >= framebuffer_ascii_cur_size) {
		size_t new_size = framebuffer_ascii_cur_size * 2;
		if (new_size < str_len + 1) new_size = str_len + 1;
		char *framebuffer_ascii_new = realloc(framebuffer_ascii, new_size);
		if (framebuffer_ascii_new == NULL) {
			return CPYMO_ERR_OUT_OF_MEM;
		}
		framebuffer_ascii = framebuffer_ascii_new;
		framebuffer_ascii_cur_size = new_size;
	}

	strcpy(framebuffer_ascii + framebuffer_ascii_cur, str);
	framebuffer_ascii_cur += str_len;

	return CPYMO_ERR_SUCC;
}

error_t cpymo_backend_image_subsys_init(void) 
{
	get_winsize(&window_size_w, &window_size_h);
	framebuffer = (uint8_t *)malloc(window_size_w * window_size_h * 3);
	if (framebuffer == NULL) return CPYMO_ERR_OUT_OF_MEM;
	
	return CPYMO_ERR_SUCC;
}

void cpymo_backend_image_subsys_free(void) 
{
	free(framebuffer);
	free(framebuffer_ascii);
}

void cpymo_backend_image_subsys_clear_framebuffer(void) 
{
	memset(framebuffer, 0, window_size_w * window_size_h * 3);
}

void cpymo_backend_image_subsys_submit_framebuffer(void) 
{
	framebuffer_ascii_cur = 0;
	printf("\033[%dA\033[%dD", (int)window_size_h, (int)window_size_w);
	for (size_t y = 0; y < window_size_h; y++) {
		for (size_t x = 0; x < window_size_w; x++) {
			put_pixel(
				framebuffer[(y * window_size_w + x) * 3 + 0],
				framebuffer[(y * window_size_w + x) * 3 + 1],
				framebuffer[(y * window_size_w + x) * 3 + 2]);
		}
		if (y < window_size_h - 1)
			cpymo_backend_image_write_string("\n");
	}

	printf("%s", framebuffer_ascii);
}

static void put_pixel_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	float r_ = (float)r / 255.0f;
	float g_ = (float)g / 255.0f;
	float b_ = (float)b / 255.0f;
	float brightness = r_ * 0.2126f + g_ * 0.7152f + b_ * 0.0722f;
	char ascii = ascii_table[(size_t)(brightness * (ascii_table_length - 1))];
	
	char str[32];
	sprintf(str, "\033[38;2;%u;%u;%um%c\033[0m", r, g, b, ascii);
	cpymo_backend_image_write_string(str);
}

typedef struct {
	uint16_t w, h, c, ow, oh;
	uint8_t px[0];
} cpymo_backend_image_impl;

error_t cpymo_backend_image_load(
	cpymo_backend_image *out_image, 
	void *pixels_moveintoimage, 
	int width, 
	int height, 
	enum cpymo_backend_image_format format)
{
	const float game_w = (float)engine.gameconfig.imagesize_w;
	const float game_h = (float)engine.gameconfig.imagesize_h;

	const uint16_t target_w = (uint16_t)((float)width / game_w * (float)window_size_w);
	const uint16_t target_h = (uint16_t)((float)height / game_h * (float)window_size_h);
	uint16_t channels;

	switch (format) {
	case cpymo_backend_image_format_rgb: channels = 3; break;
	case cpymo_backend_image_format_rgba: channels = 4; break;
	default: return CPYMO_ERR_INVALID_ARG;
	};

	cpymo_backend_image_impl *impl = 
		(cpymo_backend_image_impl *)malloc(
			sizeof(cpymo_backend_image_impl) + target_w * target_h * channels);

	if (impl == NULL) return CPYMO_ERR_OUT_OF_MEM;

	stbir_resize_uint8(
		pixels_moveintoimage, width, height, 0, 
		&impl->px[0], (int)target_w, (int)target_h, 0, (int)channels);
	impl->c = channels;
	impl->w = target_w;
	impl->h = target_h;
	impl->ow = (uint16_t)width;
	impl->oh = (uint16_t)height;
	free(pixels_moveintoimage);
	*out_image = (cpymo_backend_image *)impl;

    return CPYMO_ERR_SUCC;
}

error_t cpymo_backend_image_load_with_mask(
	cpymo_backend_image *out_image, 
	void *px_rgbx32_moveinto, 
	void *mask_a8_moveinto, 
	int w, 
	int h, 
	int mask_w, 
	int mask_h)
{
	cpymo_utils_attach_mask_to_rgba_ex(px_rgbx32_moveinto, w, h, mask_a8_moveinto, mask_w, mask_h);
	free(mask_a8_moveinto);
    return cpymo_backend_image_load(
		out_image, px_rgbx32_moveinto, w, h, cpymo_backend_image_format_rgba);
}


void cpymo_backend_image_free(cpymo_backend_image image) 
{
	free(image);
}


void cpymo_backend_image_draw(
	float dstx, float dsty, float dstw, float dsth,
	cpymo_backend_image src_,
	int srcx, int srcy, int srcw, int srch, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{
	cpymo_backend_image_impl *src = (cpymo_backend_image_impl *)src_;

	const float game_w = (float)engine.gameconfig.imagesize_w;
	const float game_h = (float)engine.gameconfig.imagesize_h;
	
	dstx /= game_w;
	dsty /= game_h;
	dstw /= game_w;
	dsth /= game_h;

	float srcx_norm = (float)srcx / (float)src->ow;
	float srcy_norm = (float)srcy / (float)src->oh;
	float srcw_norm = (float)srcw / (float)src->ow;
	float srch_norm = (float)srch / (float)src->oh;

	size_t screen_draw_w = (size_t)(dstw * window_size_w);
	size_t screen_draw_h = (size_t)(dsth * window_size_h);
	size_t screen_draw_x = (size_t)(dstx * window_size_w);
	size_t screen_draw_y = (size_t)(dsty * window_size_h);

	for (size_t draw_rect_y = 0; draw_rect_y < screen_draw_h; draw_rect_y++) {
		for (size_t draw_rect_x = 0; draw_rect_x < screen_draw_w; draw_rect_x++) {
			float pixel_alpha = alpha;

			float draw_rect_norm_x = (float)draw_rect_x / (float)screen_draw_w;
			float draw_rect_norm_y = (float)draw_rect_y / (float)screen_draw_h;

			float src_x_norm = srcx_norm + srcw_norm * draw_rect_norm_x;
			float src_y_norm = srcy_norm + srch_norm * draw_rect_norm_y;

			size_t src_x = (size_t)(src_x_norm * (src->w - 1));
			size_t src_y = (size_t)(src_y_norm * (src->h - 1));
			uint8_t *px = &src->px[(src_y * src->w + src_x) * src->c];
			
			if (src->c >= 4) {
				pixel_alpha *= px[3] / 255.0f;
			}

			float r = px[0] / 255.0f;
			float g = px[1] / 255.0f;
			float b = px[2] / 255.0f;

			size_t draw_x = screen_draw_x + draw_rect_x;
			size_t draw_y = screen_draw_y + draw_rect_y;

			if (draw_x >= window_size_w || draw_y >= window_size_h) continue;

			uint8_t *dst_px = &framebuffer[3 * (draw_y * window_size_w + draw_x)];
			float dst_r = dst_px[0] / 255.0f;
			float dst_g = dst_px[1] / 255.0f;
			float dst_b = dst_px[2] / 255.0f;

			float blend_r = dst_r * (1.0f - pixel_alpha) + r * pixel_alpha;
			float blend_g = dst_g * (1.0f - pixel_alpha) + g * pixel_alpha;
			float blend_b = dst_b * (1.0f - pixel_alpha) + b * pixel_alpha;

			blend_r = cpymo_utils_clampf(blend_r, 0, 1);
			blend_g = cpymo_utils_clampf(blend_g, 0, 1);
			blend_b = cpymo_utils_clampf(blend_b, 0, 1);

			dst_px[0] = (uint8_t)(blend_r * 255.0f);
			dst_px[1] = (uint8_t)(blend_g * 255.0f);
			dst_px[2] = (uint8_t)(blend_b * 255.0f);
		}
	}

}

void cpymo_backend_image_fill_rects(
	const float *xywh, size_t count,
	cpymo_color color, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{
	const float game_w = (float)engine.gameconfig.imagesize_w;
	const float game_h = (float)engine.gameconfig.imagesize_h;

	for (size_t i = 0; i < count; i++) {
		float x = xywh[4 * i + 0];
		float y = xywh[4 * i + 1];
		float w = xywh[4 * i + 2];
		float h = xywh[4 * i + 3];

		size_t draw_w = (size_t)(w * window_size_w / game_w);
		size_t draw_h = (size_t)(h * window_size_h / game_h);
		size_t draw_x = (size_t)(x * window_size_w / game_w);
		size_t draw_y = (size_t)(y * window_size_h / game_h);

		for (size_t draw_rect_y = 0; draw_rect_y < draw_h; draw_rect_y++) {
			for (size_t draw_rect_x = 0; draw_rect_x < draw_w; draw_rect_x++) {
				float r = (float)color.r / 255.0f;
				float g = (float)color.g / 255.0f;
				float b = (float)color.b / 255.0f;

				size_t px_x = draw_x + draw_rect_x;
				size_t px_y = draw_y + draw_rect_y;

				if (px_x >= window_size_w || px_y >= window_size_h) continue;

				uint8_t *dst_px = &framebuffer[3 * (px_y * window_size_w + px_x)];
				float dst_r = dst_px[0] / 255.0f;
				float dst_g = dst_px[1] / 255.0f;
				float dst_b = dst_px[2] / 255.0f;

				float blend_r = dst_r * (1.0f - alpha) + r * alpha;
				float blend_g = dst_g * (1.0f - alpha) + g * alpha;
				float blend_b = dst_b * (1.0f - alpha) + b * alpha;

				blend_r = cpymo_utils_clampf(blend_r, 0, 1);
				blend_g = cpymo_utils_clampf(blend_g, 0, 1);
				blend_b = cpymo_utils_clampf(blend_b, 0, 1);

				dst_px[0] = (uint8_t)(blend_r * 255.0f);
				dst_px[1] = (uint8_t)(blend_g * 255.0f);
				dst_px[2] = (uint8_t)(blend_b * 255.0f);
			}
		}
	}
}

bool cpymo_backend_image_album_ui_writable()
{
	return true;
}

error_t cpymo_backend_masktrans_create(cpymo_backend_masktrans *out, void *mask_singlechannel_moveinto, int w, int h)
{
    return CPYMO_ERR_UNSUPPORTED;
}

void cpymo_backend_masktrans_free(cpymo_backend_masktrans m){}

void cpymo_backend_masktrans_draw(cpymo_backend_masktrans m, float t, bool is_fade_in){}

