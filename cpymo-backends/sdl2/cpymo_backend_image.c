#include <cpymo_prelude.h>
#include "cpymo_backend_image.h"
#include "cpymo_import_sdl2.h"
#include <assert.h>
#include "cpymo_utils.h"

extern SDL_Renderer * renderer;

#ifdef ENABLE_SCREEN_FORCE_CENTERED
#include <cpymo_engine.h>
extern cpymo_engine engine;
#endif

#ifdef LEAKCHECK
static int leakcheck_images = 0;
void cpymo_backend_image_leakcheck(void)
{
	if (leakcheck_images) {
		printf("cpymo_backend_image LEAKS: %d\n", leakcheck_images);
	}
}
#endif

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

	#ifdef LEAKCHECK
	leakcheck_images++;
	#endif

	return CPYMO_ERR_SUCC;
}

error_t cpymo_backend_image_load_with_mask(
	cpymo_backend_image *out_image, void *px_rgbx32_moveinto, void *mask_a8_moveinto, int w, int h, int mask_w, int mask_h)
{
	cpymo_utils_attach_mask_to_rgba_ex(
        px_rgbx32_moveinto, w, h, mask_a8_moveinto, mask_w, mask_h);
        
	error_t err = cpymo_backend_image_load(
        out_image, px_rgbx32_moveinto, w, h, cpymo_backend_image_format_rgba);

    if (err == CPYMO_ERR_SUCC) 
        free(mask_a8_moveinto);
    
    return err;
}

void cpymo_backend_image_free(cpymo_backend_image image)
{
	SDL_DestroyTexture((SDL_Texture *)image);
	#ifdef LEAKCHECK
	leakcheck_images--;
	#endif
}

#ifdef ENABLE_SCREEN_FORCE_CENTERED
static void cpymo_backend_image_force_center(
	float *dstx, float *dsty, float *dstw, float *dsth) {
	/*float offset = (float)(SCREEN_WIDTH - engine.gameconfig.imagesize_w) / 2;
	*dstx += offset;
	offset = (float)(SCREEN_HEIGHT - engine.gameconfig.imagesize_h) / 2;
	*dsty += offset;*/

	float game_w = engine.gameconfig.imagesize_w;
	float game_h = engine.gameconfig.imagesize_h;
	cpymo_utils_match_rect(
		SCREEN_WIDTH, SCREEN_HEIGHT,
		&game_w, &game_h);

	*dstw /= engine.gameconfig.imagesize_w;
	*dstw *= game_w;
	*dsth /= engine.gameconfig.imagesize_h;
	*dsth *= game_h;

	*dstx /= engine.gameconfig.imagesize_w;
	*dstx *= game_w;
	*dsty /= engine.gameconfig.imagesize_h;
	*dsty *= game_h;
	cpymo_utils_center(SCREEN_WIDTH, SCREEN_HEIGHT, game_w, game_h, dstx, dsty);
}
#endif

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
#ifdef ENABLE_SCREEN_FORCE_CENTERED
	cpymo_backend_image_force_center(&dstx, &dsty, &dstw, &dsth);
#endif

#ifdef DISABLE_IMAGE_SCALING
	assert(dstw == (int)srcw);
	assert(dsth == (int)srch);
#endif
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
#if SDL_VERSION_ATLEAST(2, 0, 10) && !defined ENABLE_SCREEN_FORCE_CENTERED
	if (SDL_RenderFillRectsF(renderer, (SDL_FRect *)xywh, (int)count) != 0)
		SDL_Log("Warning: SDL_RenderFillRectsF failed, %s", SDL_GetError());
#else 
	for (size_t i = 0; i < count; ++i) {
		SDL_Rect rect;
		rect.x = (int)xywh[i * 4];
		rect.y = (int)xywh[i * 4 + 1];
		rect.w = (int)xywh[i * 4 + 2];
		rect.h = (int)xywh[i * 4 + 3];

#ifdef ENABLE_SCREEN_FORCE_CENTERED
		float rx = rect.x, ry = rect.y, rw = rect.w, rh = rect.h;
		cpymo_backend_image_force_center(&rx, &ry, &rw, &rh);
		rect.x = (int)rx;
		rect.y = (int)ry;
		rect.w = (int)rw; 
		rect.h = (int)rh;
#endif
		SDL_RenderFillRect(renderer, &rect);
	}
#endif
}

bool cpymo_backend_image_album_ui_writable() { return true; }

#ifdef ENABLE_SDL2_IMAGE
#include <cpymo_package.h>
#include <cpymo_assetloader.h>
#include <SDL2/SDL_image.h>

error_t cpymo_assetloader_load_icon_pixels(
	void **px, int *w, int *h, const char *gamedir)
{
	return CPYMO_ERR_UNSUPPORTED;
}

error_t cpymo_assetloader_load_icon(
	cpymo_backend_image *out, int *w, int *h, const char *gamedir)
{
	char *path = alloca(strlen(gamedir) + 10);
	sprintf(path, "%s/icon.png", gamedir);
	SDL_Texture *t = IMG_LoadTexture(renderer, path);
	if (t == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);

	if (SDL_QueryTexture(t, NULL, NULL, w, h)) {
		SDL_DestroyTexture(t);
		return CPYMO_ERR_UNKNOWN;
	}

	*out = (cpymo_backend_image)t;
	return CPYMO_ERR_SUCC;
}

static SDL_Surface *cpymo_package_sdl2_load_surface(
	const cpymo_package *package, cpymo_str filename)
{
	cpymo_package_index index;
	error_t err = cpymo_package_find(&index, package, filename);
	if (err != CPYMO_ERR_SUCC) return NULL;

	cpymo_package_stream_reader r = cpymo_package_stream_reader_create(
		package, &index);
	fseek(r.stream, r.file_offset, SEEK_SET);

	SDL_RWops *rw = SDL_RWFromFP(r.stream, false);
	if (rw == NULL) return NULL;

	return IMG_Load_RW(rw, true);
}

static void cpymo_assetloader_sdl2_attach_mask(
	SDL_Surface **img,
	const cpymo_package *pkg, bool use_pkg,
	const cpymo_assetloader *loader,
	const char *asset_type,
	cpymo_str asset_name,
	const char *mask_ext)
{
	SDL_Surface *mask = NULL;
	char *name = alloca(asset_name.len + 8);
	cpymo_str_copy(name, asset_name.len + 8, asset_name);
	strcat(name, "_mask");

	if (use_pkg) {
		mask = cpymo_package_sdl2_load_surface(
			pkg, cpymo_str_pure(name));
	}
	else {
		char *path = NULL;
		error_t err = cpymo_assetloader_get_fs_path(
			&path, cpymo_str_pure(name),
			asset_type, mask_ext, loader);
		
		if (err != CPYMO_ERR_SUCC) return;

		mask = IMG_Load(path);
		free(path);
	}

	if (mask == NULL) return;
	if (mask->w != (*img)->w || mask->h != (*img)->h) {
		SDL_FreeSurface(mask);
		return;
	}

	SDL_Surface *mask2 = SDL_ConvertSurfaceFormat(
		mask, SDL_PIXELFORMAT_RGBA8888, 0);
	SDL_FreeSurface(mask);
	if (mask2 == NULL) return;
	mask = mask2;

	SDL_Surface *img_rgba = SDL_ConvertSurfaceFormat(
		*img, SDL_PIXELFORMAT_RGBA8888, 0);

	if (img_rgba == NULL) {
		SDL_FreeSurface(mask);
		return;
	}

	for (int y = 0; y < img_rgba->h; ++y) {
		for (int x = 0; x < img_rgba->w; ++x) {
			uint8_t *img_rgba_px = 
				((uint8_t *)img_rgba->pixels)
				+ ((size_t)y * img_rgba->pitch)
				+ ((size_t)x * img_rgba->format->BytesPerPixel);

			uint8_t *mask_px = 
				((uint8_t *)mask->pixels)
				+ ((size_t)y * mask->pitch)
				+ ((size_t)x * mask->format->BytesPerPixel);

			Uint8 dummy;
			SDL_GetRGBA(*(Uint32 *)mask_px, mask->format,
				&img_rgba_px[0], &dummy, &dummy, &dummy);

			img_rgba_px[0] = mask_px[2];
		}
	}

	SDL_FreeSurface(mask);
	SDL_FreeSurface(*img);
	*img = img_rgba;
}

error_t cpymo_assetloader_load_image_with_mask(
	cpymo_backend_image *img, int *w, int *h, 
	cpymo_str name, 
	const char *asset_type,
	const char *asset_ext,
	const char *mask_ext,
	bool use_pkg,
	const cpymo_package *pkg,
	const cpymo_assetloader *loader,
	bool load_mask)
{
	SDL_Surface *sur = NULL;
	if (use_pkg) {
		sur = cpymo_package_sdl2_load_surface(pkg, name);
	}
	else {
		char *path = NULL;
		error_t err = cpymo_assetloader_get_fs_path(
			&path,
			name,
			asset_type,
			asset_ext,
			loader);
		CPYMO_THROW(err);

		sur = IMG_Load(path);
		free(path);
	}

	if (sur == NULL) 
		return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	if (load_mask && cpymo_gameconfig_is_symbian(loader->game_config)) 
		cpymo_assetloader_sdl2_attach_mask(
			&sur, pkg, use_pkg, loader, asset_type, name, mask_ext);

	SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, sur);
	int sw = sur->w, sh = sur->h;
	SDL_FreeSurface(sur);

	if (t == NULL) return CPYMO_ERR_UNKNOWN;

	SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);

	*img = (cpymo_backend_image)t;
	*w = sw;
	*h = sh;
	return CPYMO_ERR_SUCC;
}

error_t cpymo_assetloader_load_bg_pixels(
	void ** px, int * w, int * h, 
	cpymo_str name, const cpymo_assetloader * loader)
{ return CPYMO_ERR_UNSUPPORTED; }

error_t cpymo_assetloader_load_bg_image(
	cpymo_backend_image * img, int * w, int * h, 
	cpymo_str name, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_image_with_mask(
		img, w, h, name, "bg", loader->game_config->bgformat, "",
		loader->use_pkg_bg, &loader->pkg_bg, loader, false);
}

error_t cpymo_assetloader_load_system_masktrans(
	cpymo_backend_masktrans *out, cpymo_str name, 
	const cpymo_assetloader *loader)
{ 
	char *path = NULL;
	error_t err = cpymo_assetloader_get_fs_path(
		&path, name, "system", "png", loader);
	CPYMO_THROW(err);

	SDL_Surface *sur = IMG_Load(path);
	if (sur == NULL) {
		free(path);
		return CPYMO_ERR_CAN_NOT_OPEN_FILE;
	}

	SDL_Surface *sur2 = 
		SDL_ConvertSurfaceFormat(sur, SDL_PIXELFORMAT_RGBA8888, 0);
	SDL_FreeSurface(sur);
	if (sur2 == NULL) return CPYMO_ERR_OUT_OF_MEM;

	uint8_t *px = malloc(sur2->w * sur2->h);
	if (px == NULL) {
		SDL_FreeSurface(sur2);
		return CPYMO_ERR_OUT_OF_MEM;
	}

	for (int y = 0; y < sur2->h; ++y) {
		for (int x = 0; x < sur2->w; ++x) {
			uint8_t *px_ = 
				((uint8_t *)sur2->pixels)
				+ ((size_t)y * sur2->pitch)
				+ ((size_t)x * sur2->format->BytesPerPixel);
			px[y * sur2->w + x] = px_[3];
		}
	}

	int w = sur2->w, h = sur2->h;
	SDL_FreeSurface(sur2);
	
	return cpymo_backend_masktrans_create(out, px, w, h);
}

#endif


