#include "cpymo_backend_image.h"
#include <SDL.h>
#include <assert.h>
#include "cpymo_utils.h"

extern SDL_Renderer *renderer;

error_t cpymo_backend_image_load(
	cpymo_backend_image *out_image, void *px, int w, int h, enum cpymo_backend_image_format fmt)
{
	int channels;

	switch (fmt) {
	case cpymo_backend_image_format_r: channels = 1; break;
	case cpymo_backend_image_format_rgb: channels = 3; break;
	case cpymo_backend_image_format_rgba: channels = 4; break;
	default: assert(false);
	}

	SDL_Surface *surface =
		SDL_CreateRGBSurfaceFrom(
			px,
			w, 
			h, 
			channels * 8, 
			channels * w, 
			0x000000FF, 
			0x0000FF00, 
			0x00FF0000, 
			0xFF000000);

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
	cpymo_backend_image *out_image, void *px_rgbx32_moveinto, void *mask_a8_moveinto, int w, int h)
{
	cpymo_utils_attach_mask_to_rgba(px_rgbx32_moveinto, mask_a8_moveinto, w, h);
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

	SDL_FRect dst_rect;
	dst_rect.x = dstx;
	dst_rect.y = dsty;
	dst_rect.w = dstw;
	dst_rect.h = dsth;

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
	if (SDL_RenderFillRectsF(renderer, (SDL_FRect *)xywh, (int)count) != 0)
		SDL_Log("Warning: SDL_RenderFillRectsF failed, %s", SDL_GetError());
}

/*error_t cpymo_backend_image_create_mutable(cpymo_backend_image *out_image, int width, int height)
{
	SDL_Texture *tex =
		SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);

	if (tex == NULL) return CPYMO_ERR_UNKNOWN;

	SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

	*out_image = tex;
	return CPYMO_ERR_SUCC;
}

error_t cpymo_backend_image_update_mutable(cpymo_backend_image image, void * pixels_moveintoimage)
{
	int w;
	error_t err = cpymo_backend_image_size(image, &w, NULL);
	if (err != CPYMO_ERR_SUCC) return err;

	if (SDL_UpdateTexture((SDL_Texture *)image, NULL, pixels_moveintoimage, w * 4) != 0) {
		SDL_Log("Warning: Can not update texture, %s", SDL_GetError());
		return CPYMO_ERR_UNKNOWN;
	}

	free(pixels_moveintoimage);

	return CPYMO_ERR_SUCC;
}*/
