#include "cpymo_backend_image.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include "cpymo_utils.h"

extern SDL_Renderer *renderer;

error_t cpymo_backend_image_load(
	cpymo_backend_image *out_image, void *px, int w, int h, enum cpymo_backend_image_format fmt)
{
	int channels;

	switch (fmt) {
	case cpymo_backend_image_format_rgb: channels = 3; break;
	case cpymo_backend_image_format_rgba: channels = 4; break;
	default: assert(false); return CPYMO_ERR_UNKNOWN;
	}

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

	SDL_Surface *surface =
		SDL_CreateRGBSurfaceFrom(
			px,
			w, 
			h, 
			channels * 8, 
			channels * w, 
			rmask, 
			gmask, 
			bmask, 
			amask);

	if (surface == NULL) {
		SDL_Log("Warning: Can not load image: %s", SDL_GetError());
		return CPYMO_ERR_UNKNOWN;
	}

	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

	SDL_FreeSurface(surface);

	if (tex == NULL) {
		SDL_Log("Warning: Can not load texture: %s", SDL_GetError());
		return CPYMO_ERR_UNKNOWN;
	}

	*out_image = (cpymo_backend_image)tex;
	free(px);
	return CPYMO_ERR_SUCC;
}

error_t cpymo_backend_image_load_with_mask(
	cpymo_backend_image *out_image, void *px_rgbx32_moveinto, void *mask_a8_moveinto, int w, int h, int mask_w, int mask_h)
{
	cpymo_utils_attach_mask_to_rgba_ex(px_rgbx32_moveinto, w, h, mask_a8_moveinto, mask_w, mask_h);
	free(mask_a8_moveinto);
	return cpymo_backend_image_load(out_image, px_rgbx32_moveinto, w, h, cpymo_backend_image_format_rgba);
}

void cpymo_backend_image_free(cpymo_backend_image image)
{
	SDL_DestroyTexture((SDL_Texture *)image);
}

void cpymo_backend_image_draw(
	float dstx, 
	float dsty, 
	float dstw, 
	float dsth, 
	cpymo_backend_image src, 
	int srcx, 
	int srcy, 
	int srcw, 
	int srch, 
	float alpha, 
	enum cpymo_backend_image_draw_type draw_type)
{
	SDL_Rect src_rect;
	src_rect.x = srcx;
	src_rect.y = srcy;
	src_rect.w = srcw;
	src_rect.h = srch;

#if SDL_VERSION_ATLEAST(2, 0, 10)
	SDL_FRect dst_rect;
	dst_rect.x = dstx;
	dst_rect.y = dsty;
	dst_rect.w = dstw;
	dst_rect.h = dsth;
#else
	SDL_Rect dst_rect;
	dst_rect.x = (int)dstx;
	dst_rect.y = (int)dsty;
	dst_rect.w = (int)dstw;
	dst_rect.h = (int)dsth;
#define SDL_RenderCopyF SDL_RenderCopy
#endif

	const Uint8 a = (Uint8)(alpha * 255);
	SDL_SetTextureAlphaMod((SDL_Texture *)src, a);

	if (0 != SDL_RenderCopyF(
		renderer,
		(SDL_Texture *)src,
		&src_rect,
		&dst_rect
	)) {
		SDL_Log("Warning: SDL_RenderCopyF failed: %s", SDL_GetError());
	}
}

void cpymo_backend_image_fill_rects(const float * xywh, size_t count, cpymo_color color, float alpha, enum cpymo_backend_image_draw_type draw_type)
{
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, (Uint8)(alpha * 255));
#if SDL_VERSION_ATLEAST(2, 0, 10)
	if (SDL_RenderFillRectsF(renderer, (SDL_FRect *)xywh, (int)count) != 0)
		SDL_Log("Warning: SDL_RenderFillRectsF failed, %s", SDL_GetError());
#else 
	for (size_t i = 0; i < count; ++i) {
		SDL_Rect rect;
		rect.x = (int)xywh[i * 4];
		rect.y = (int)xywh[i * 4 + 1];
		rect.w = (int)xywh[i * 4 + 2];
		rect.h = (int)xywh[i * 4 + 3];
		SDL_RenderFillRect(renderer, &rect);
	}
#endif
}

bool cpymo_backend_image_album_ui_writable() { return true; }