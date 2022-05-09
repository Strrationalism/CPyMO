#include <cpymo_backend_image.h>
#include <cpymo_backend_masktrans.h>
#include <stdlib.h>
#include <SDL.h>
#include <cpymo_engine.h>

const extern cpymo_engine engine;
extern SDL_Surface *framebuffer;

error_t cpymo_backend_image_load(
	cpymo_backend_image *out_image, 
	void *pixels_moveintoimage, 
	int width, 
	int height, 
	enum cpymo_backend_image_format format)
{
	Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

	int channels = format == cpymo_backend_image_format_rgb ? 3 : 4;
	SDL_Surface *sur = SDL_CreateRGBSurfaceFrom(
		pixels_moveintoimage,
		width,
		height,
		channels * 8,
		width * channels,
		rmask,
		gmask,
		bmask,
		amask);

	if (sur == NULL) {
		return CPYMO_ERR_UNKNOWN;
	}

	SDL_Surface *sur2;
	if (format == cpymo_backend_image_format_rgb)
		sur2 = SDL_DisplayFormat(sur);
	else sur2 = SDL_DisplayFormatAlpha(sur);

	SDL_FreeSurface(sur);
	free(pixels_moveintoimage);

	if (sur2 == NULL) return CPYMO_ERR_UNKNOWN;

	*out_image = (cpymo_backend_image)sur2;

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
	cpymo_utils_attach_mask_to_rgba_ex(
		px_rgbx32_moveinto, w, h, 
		mask_a8_moveinto, mask_w, mask_h);
	free(mask_a8_moveinto);
	return cpymo_backend_image_load(
		out_image, px_rgbx32_moveinto, w, h, cpymo_backend_image_format_rgba);
}

void cpymo_backend_image_free(cpymo_backend_image image) 
{
	SDL_FreeSurface((SDL_Surface *)image);
}

static void trans_pos(float *x, float *y) 
{
	float screen_w = (float)framebuffer->w;
	float screen_h = (float)framebuffer->h;

	float game_w = (float)engine.gameconfig.imagesize_w;
	float game_h = (float)engine.gameconfig.imagesize_h;

	*x = *x + (screen_w - game_w) / 2;
	*y = *y + (screen_h - game_h) / 2;
}

void cpymo_backend_image_draw(
	float dstx, float dsty, float dstw, float dsth,
	cpymo_backend_image src_,
	int srcx, int srcy, int srcw, int srch, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{
	SDL_Surface *src = (SDL_Surface *)src_;
	trans_pos(&dstx, &dsty);

	SDL_Rect srcrect, dstrect;
	srcrect.x = (Sint16)srcx;
	srcrect.y = (Sint16)srcy;
	srcrect.w = (Uint16)srcw;
	srcrect.h = (Uint16)srch;
	dstrect.x = (Sint16)dstx;
	dstrect.y = (Sint16)dsty;
	dstrect.w = (Uint16)dstw;
	dstrect.h = (Uint16)dsth;

	SDL_SetAlpha(src, SDL_SRCALPHA, (Uint8)(alpha * 255));
	
	SDL_BlitSurface(src, &srcrect, framebuffer, &dstrect);
}

void cpymo_backend_image_fill_rects(
	const float *xywh, size_t count,
	cpymo_color color, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{
	Uint32 col = SDL_MapRGBA(
		framebuffer->format, 
		(Uint8)(color.r * alpha), (Uint8)(color.g * alpha), (Uint8)(color.b * alpha), 255);

	for (size_t i = 0; i < count; ++i) {
		SDL_Rect rect;
		float x = xywh[i * 4 + 0];
		float y = xywh[i * 4 + 1];
		trans_pos(&x, &y);
		rect.x = (Sint16)x;
		rect.y = (Sint16)y;
		rect.w = (Uint16)xywh[i * 4 + 2];
		rect.h = (Uint16)xywh[i * 4 + 3];

		SDL_FillRect(framebuffer, &rect, col);
	}
}

bool cpymo_backend_image_album_ui_writable()
{
	return true;
}

error_t cpymo_backend_masktrans_create(cpymo_backend_masktrans *out, void *mask_singlechannel_moveinto, int w, int h)
{
	if (w != framebuffer->w || h != framebuffer->h)
    	return CPYMO_ERR_UNSUPPORTED;

	*out = (cpymo_backend_masktrans)mask_singlechannel_moveinto;
	return CPYMO_ERR_SUCC;
}

void cpymo_backend_masktrans_free(cpymo_backend_masktrans m) { free(m); }

void cpymo_backend_masktrans_draw(cpymo_backend_masktrans m, float t, bool is_fade_in)
{
	if (SDL_LockSurface(framebuffer) == -1) return;

	const float radius = 0.25f;
	t = t * (1.0f + 2 * radius) - radius;

	float t_top = t + radius;
	float t_bottom = t - radius;

	for (size_t y = 0; y < framebuffer->h; ++y) {
		for (size_t x = 0; x < framebuffer->w; ++x) {
			size_t px_offset = framebuffer->pitch * y + x * framebuffer->format->BytesPerPixel;
			Uint8 *px = (Uint8 *)framebuffer->pixels + px_offset;

			float mask =
				(float)(((unsigned char*)m)[y * framebuffer->w + x]) / 255;

			if (mask > t_top) mask = 1.0f;
			else if (mask < t_bottom) mask = 0.0f;
			else mask = (mask - t_bottom) / (2 * radius);

			if (is_fade_in) mask = 1 - mask;

			Uint32 col;
			switch (framebuffer->format->BytesPerPixel) {
			case 1: col = *px; break;
			case 2: col = *(Uint16 *)px; break;
			case 3: col = (*(Uint32 *)px) >> 16; break;
			case 4: col = *(Uint32 *)px; break;
			default: 
				SDL_UnlockSurface(framebuffer);
				return;
			};

			Uint8 r, g, b;
			SDL_GetRGB(col, framebuffer->format, &r, &g, &b);

			r = (Uint8)(r * mask);
			g = (Uint8)(g * mask);
			b = (Uint8)(b * mask);

			col = SDL_MapRGB(framebuffer->format, r, g, b);

			switch (framebuffer->format->BytesPerPixel) {
			case 1: *px = (Uint8)col; break;
			case 2: *(Uint16 *)px = (Uint16)col; break;
			case 3: 
				*(Uint8 *)px = (Uint8)(col >> 16);
				*(Uint8 *)(px + 1) = (Uint8)(col >> 8);
				*(Uint8 *)(px + 2) = (Uint8)col;
				break;
			case 4: *(Uint32 *)px = (Uint32)col; break;
			}
		}
	}

	SDL_UnlockSurface(framebuffer);
}
