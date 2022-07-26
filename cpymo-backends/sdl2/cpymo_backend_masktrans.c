#include <cpymo_backend_masktrans.h>
#include "cpymo_import_sdl2.h"
#include <cpymo_backend_image.h>
#include <memory.h>
#include <assert.h>
#include <stdlib.h>

#include <cpymo_engine.h>
extern cpymo_engine engine;


typedef struct {
	void *mask;
	SDL_Texture *tex;
	int w, h;
} cpymo_backend_masktrans_i;

extern SDL_Renderer *renderer;

error_t cpymo_backend_masktrans_create(cpymo_backend_masktrans *out, void *mask_singlechannel_moveinto, int w, int h)
{
	cpymo_backend_masktrans_i *m = 
		(cpymo_backend_masktrans_i *)malloc(sizeof(cpymo_backend_masktrans_i));

	if (m == NULL) return CPYMO_ERR_OUT_OF_MEM;

	m->tex =
		SDL_CreateTexture(
			renderer, 
			SDL_PIXELFORMAT_RGBA8888, 
			SDL_TEXTUREACCESS_STREAMING, 
			w, h);

	SDL_SetTextureBlendMode(m->tex, SDL_BLENDMODE_BLEND);

	if (m->tex == NULL) {
		free(m);
		return CPYMO_ERR_OUT_OF_MEM;
	}

	m->mask = mask_singlechannel_moveinto;
	m->w = w;
	m->h = h;
	
	*out = m;

	return CPYMO_ERR_SUCC;
}

void cpymo_backend_masktrans_free(cpymo_backend_masktrans mt) 
{
	cpymo_backend_masktrans_i *m = (cpymo_backend_masktrans_i *)mt;
	free(m->mask);
	SDL_DestroyTexture(m->tex);
	free(mt);
}

void cpymo_backend_masktrans_draw(cpymo_backend_masktrans mt, float t, bool is_fade_in)
{
	cpymo_backend_masktrans_i *m = (cpymo_backend_masktrans_i *)mt;

	if (!is_fade_in) t = 1.0f - t;

	void *pixels;
	int pitch;
	if (SDL_LockTexture(m->tex, NULL, &pixels, &pitch) != 0)
		return;

	memset(pixels, 255, pitch * m->h);

	const float radius = 0.25f;
	t = t * (1.0f + 2 * radius) - radius;

	float t_top = t + radius;
	float t_bottom = t - radius;
	
	for (int y = 0; y < m->h; ++y) {
		for (int x = 0; x < m->w; ++x) {
			float mask =
				(float)(((unsigned char*)m->mask)[y * m->w + x]) / 255;

			if (is_fade_in) mask = 1 - mask;

			if (mask > t_top) mask = 1.0f;
			else if (mask < t_bottom) mask = 0.0f;
			else mask = (mask - t_bottom) / (2 * radius);

			Uint32 *px = (Uint32 *)&((Uint8 *)pixels)[y * pitch + x * 4];
			*px = (Uint32)(mask * 255.0f);
		}
	}

	SDL_UnlockTexture(m->tex);

	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = engine.gameconfig.imagesize_w;
	rect.h = engine.gameconfig.imagesize_h;

#ifdef ENABLE_SCREEN_FORCE_CENTERED
	rect.x += (SCREEN_WIDTH - rect.w) / 2;
	rect.y += (SCREEN_HEIGHT - rect.h) / 2;
#endif

	SDL_RenderCopy(renderer, m->tex, NULL, &rect);
}
