#include <cpymo_prelude.h>
#include <cpymo_backend_image.h>
#include <cpymo_backend_masktrans.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <cpymo_engine.h>

#ifdef ENABLE_SDL_IMAGE
#include <SDL/SDL_image.h>
#endif

const extern cpymo_engine engine;
extern SDL_Surface *framebuffer;

error_t cpymo_backend_image_init(void)
{
	#ifdef ENABLE_SDL_IMAGE
	int flags =
		IMG_INIT_JPG |
		IMG_INIT_PNG |
		IMG_INIT_TIF |
		IMG_INIT_WEBP;

	int result = IMG_Init(flags);

	if (0 == result) {
		printf("[Error] IMG_Init: %s\n", IMG_GetError());
		return CPYMO_ERR_UNKNOWN;
	}

	if (flags != result) 
		printf("[Warning] IMG_Init(%d): %s\n", result, IMG_GetError());

	#endif

	return CPYMO_ERR_SUCC;
}

void cpymo_backend_image_quit(void)
{
	#ifdef ENABLE_SDL_IMAGE
	IMG_Quit();
	#endif
}

cpymo_color getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	Uint32 px;

    switch(bpp) {
    case 1:
        px = *p;
		break;

    case 2:
        px = *(Uint16 *)p;
		break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            px = (p[0] << 16 | p[1] << 8 | p[2]);
        else
            px = (p[0] | p[1] << 8 | p[2] << 16);
		break;

    case 4:
        px = *(Uint32 *)p;
		break;

    default:
        px = 0;
		break;

    }

	cpymo_color col;
	SDL_GetRGB(px, surface->format, &col.r, &col.g, &col.b);
	return col;
}

void putpixel(SDL_Surface *surface, int x, int y, cpymo_color col, Uint8 alpha)
{
    int bpp = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	Uint32 pixel = SDL_MapRGBA(surface->format, col.r, col.g, col.b, alpha);

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	#define RMASK 0xff000000
	#define GMASK 0x00ff0000
	#define BMASK 0x0000ff00
	#define AMASK 0x000000ff
#else
	#define RMASK 0x000000ff
	#define GMASK 0x0000ff00
	#define BMASK 0x00ff0000
	#define AMASK 0xff000000
#endif

error_t cpymo_backend_image_load(
	cpymo_backend_image *out_image, 
	void *pixels_moveintoimage, 
	int width, 
	int height, 
	enum cpymo_backend_image_format format)
{
	Uint32 rmask = RMASK, gmask = GMASK, bmask = BMASK, amask = AMASK;
#ifdef __WII__
	if (format == cpymo_backend_image_format_rgb) {
		rmask >>= 8;
		gmask >>= 8;
		bmask >>= 8;
		amask >>= 8;
	}
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
        px_rgbx32_moveinto, w, h, mask_a8_moveinto, mask_w, mask_h);
        
	error_t err = cpymo_backend_image_load(
        out_image, px_rgbx32_moveinto, w, h, cpymo_backend_image_format_rgba);

    if (err == CPYMO_ERR_SUCC) 
        free(mask_a8_moveinto);
    
    return err;
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
	if (draw_type == cpymo_backend_image_draw_type_chara) {
		if (alpha < 0.75f) return;
	}

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

	int xoff = ((int)dstrect.w - (int)srcrect.w) / 2;
	int yoff = ((int)dstrect.h - (int)srcrect.h) / 2;

	if (xoff < 0) xoff = 0;
	if (yoff < 0) yoff = 0;

	dstrect.x += xoff;
	dstrect.y += yoff;

	SDL_SetAlpha(src, SDL_SRCALPHA, (Uint8)(alpha * 255));
	
	SDL_BlitSurface(src, &srcrect, framebuffer, &dstrect);
}

void cpymo_backend_image_fill_rects(
	const float *xywh, size_t count,
	cpymo_color color, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{
#ifdef FAST_FILL_RECT
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
#else
	if (SDL_LockSurface(framebuffer) == -1) return;

	SDL_Rect clip;
	SDL_GetClipRect(framebuffer, &clip);

	float r = color.r / 255.0f * alpha;
	float g = color.g / 255.0f * alpha;
	float b = color.b / 255.0f * alpha;

	for (size_t i = 0; i < count; ++i) {
		float x_ = xywh[i * 4 + 0];
		float y_ = xywh[i * 4 + 1];
		int w = (int)xywh[i * 4 + 2];
		int h = (int)xywh[i * 4 + 3];
		trans_pos(&x_, &y_);

		for (int y = (int)y_; y < h + (int)y_; ++y) {
			for (int x = (int)x_; x < w + (int)x_; x++) {
				if (x < 0 || x >= framebuffer->w || y < 0 || y >= framebuffer->h) continue;
				if (x < clip.x || x >= clip.x + clip.w || y < clip.y || y >= clip.y + clip.h) continue;

				cpymo_color dst = getpixel(framebuffer, x, y);
				float dst_r = dst.r / 255.0f * (1 - alpha) + r;
				float dst_g = dst.g / 255.0f * (1 - alpha) + g;
				float dst_b = dst.b / 255.0f * (1 - alpha) + b;

				dst_r = cpymo_utils_clampf(dst_r, 0, 1);
				dst_g = cpymo_utils_clampf(dst_g, 0, 1);
				dst_b = cpymo_utils_clampf(dst_b, 0, 1);

				dst.r = (Uint8)(dst_r * 255);
				dst.g = (Uint8)(dst_g * 255);
				dst.b = (Uint8)(dst_b * 255);
				putpixel(framebuffer, x, y, dst, 255);
			}
		}
	}

	SDL_UnlockSurface(framebuffer);
#endif
}

bool cpymo_backend_image_album_ui_writable()
{
	return true;
}

error_t cpymo_backend_masktrans_create(cpymo_backend_masktrans *out, void *mask_singlechannel_moveinto, int w, int h)
{
	if (w != engine.gameconfig.imagesize_w || h != engine.gameconfig.imagesize_h)
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

	SDL_Rect rect;
	SDL_GetClipRect(framebuffer, &rect);

	for (int y = 0; y < engine.gameconfig.imagesize_h; ++y) {
		for (int x = 0; x < engine.gameconfig.imagesize_w; ++x) {
			int px_x = x + rect.x;
			int px_y = y + rect.y;

			if (px_x < rect.x || px_x >= rect.x + rect.w || px_y < rect.y || px_y >= rect.y + rect.h) continue;

			float mask =
				(float)(((unsigned char*)m)[y * engine.gameconfig.imagesize_w + x]) / 255;

			if (mask > t_top) mask = 1.0f;
			else if (mask < t_bottom) mask = 0.0f;
			else mask = (mask - t_bottom) / (2 * radius);

			if (is_fade_in) mask = 1 - mask;

			cpymo_color col = getpixel(framebuffer, px_x, px_y);

			col.r = (uint8_t)(col.r * mask);
			col.g = (uint8_t)(col.g * mask);
			col.b = (uint8_t)(col.b * mask);
			putpixel(framebuffer, px_x, px_y, col, 255);
		}
	}

	SDL_UnlockSurface(framebuffer);
}

#ifdef ENABLE_SDL_IMAGE
#include <cpymo_package.h>
#include <cpymo_assetloader.h>

error_t cpymo_assetloader_load_icon_pixels(
	void **px, int *w, int *h, const char *gamedir)
{
	return CPYMO_ERR_UNSUPPORTED;
}

error_t cpymo_assetloader_load_icon(
	cpymo_backend_image *out, int *w, int *h, const char *gamedir)
{
	char *filename = (char *)malloc(strlen(gamedir) + 10);
	if (filename == NULL) return CPYMO_ERR_OUT_OF_MEM;
	sprintf(filename, "%s/icon.png", gamedir);
	SDL_Surface *sur = IMG_Load(filename);
	free(filename);
	if (sur == NULL) 
		return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	*w = sur->w;
	*h = sur->h;
	
	SDL_Surface *sur_optimized = SDL_DisplayFormat(sur);
	if (sur_optimized == NULL) {
		*out = (cpymo_backend_image)sur;
		return CPYMO_ERR_SUCC;
	}

	SDL_FreeSurface(sur);
	*out = (cpymo_backend_image)sur_optimized;

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_assetloader_load_image_surface(
	SDL_Surface **out,
	cpymo_str name,
	const char *asset_type,
	const char *asset_ext,
	bool use_pkg,
	const cpymo_package *pkg,
	const cpymo_assetloader *loader)
{
	SDL_Surface *sur;
	if (use_pkg) {
		cpymo_package_stream_reader r;
		error_t err = cpymo_package_stream_reader_find_create(&r, pkg, name);
		CPYMO_THROW(err);

		SDL_RWops *rwops = SDL_RWFromFP(r.stream, 0);
		if (rwops == NULL) {
			cpymo_package_stream_reader_close(&r);
			return CPYMO_ERR_OUT_OF_MEM;
		}

		sur = IMG_Load_RW(rwops, 0);
		SDL_RWclose(rwops);
		cpymo_package_stream_reader_close(&r);
		
		/*char *buf;
		size_t len;
		error_t err = cpymo_package_read_file(&buf, &len, pkg, name);
		CPYMO_THROW(err);

		SDL_RWops *rwops = SDL_RWFromConstMem(buf, (int)len);
		if (rwops == NULL) {
			free(buf);
			return CPYMO_ERR_OUT_OF_MEM;
		}

		sur = IMG_Load_RW(rwops, 0);
		SDL_RWclose(rwops);
		free(buf);*/
	}
	else {
		char *path;
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

	if (sur == NULL) return CPYMO_ERR_OUT_OF_MEM;
	*out = sur;
	return CPYMO_ERR_SUCC;
}

static error_t cpymo_assetloader_load_image_with_mask_ex(
	cpymo_backend_image *img, int *w, int *h, 
	cpymo_str name, 
	const char *asset_type,
	const char *asset_ext,
	const char *mask_ext,
	bool use_pkg,
	const cpymo_package *pkg,
	const cpymo_assetloader *loader,
	bool load_mask,
	SDL_Surface *(*display_format_converter)(SDL_Surface *))
{
	SDL_Surface *sur;
	error_t err = cpymo_assetloader_load_image_surface(
		&sur, name, asset_type, asset_ext, use_pkg, pkg, loader);
	CPYMO_THROW(err);

	if (sur == NULL) return CPYMO_ERR_OUT_OF_MEM;

	if (load_mask) {
		char *mask_name = (char *)malloc(name.len + 6);
		if (mask_name) {
			cpymo_str_copy(mask_name, name.len + 6, name);
			strcat(mask_name, "_mask");

			SDL_Surface *mask;
			error_t errmask = cpymo_assetloader_load_image_surface(
				&mask, cpymo_str_pure(mask_name), asset_type, mask_ext, 
				use_pkg, pkg, loader);
			free(mask_name);

			if (errmask == CPYMO_ERR_SUCC) {
				SDL_Surface *masked_image = SDL_CreateRGBSurface(
					0, 
					sur->w, 
					sur->h, 
					32, 
					RMASK, 
					GMASK, 
					BMASK, 
					AMASK);
				
				if (masked_image) {
					for (int y = 0; y < sur->h; ++y) {
						for (int x = 0; x < sur->w; ++x) {
							cpymo_color maskp = getpixel(sur, x, y);
							cpymo_color maskc = getpixel(
								mask, 
								(int)((float)x / (float)sur->w * (mask->w - 1)),
								(int)((float)y / (float)sur->h * (mask->h - 1)));
							putpixel(masked_image, x, y, maskp, maskc.r);
						}
					}

					SDL_FreeSurface(sur);
					sur = masked_image;
				}

				SDL_FreeSurface(mask);
			}
		}
	}

	*w = sur->w;
	*h = sur->h;

	SDL_Surface *sur_optimized = display_format_converter(sur);
	if (sur_optimized) {
		SDL_FreeSurface(sur);
		sur = sur_optimized;
	}

	*img = (cpymo_backend_image)sur;

	return CPYMO_ERR_SUCC;
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
	return cpymo_assetloader_load_image_with_mask_ex(
		img, w, h, name, asset_type, asset_ext, mask_ext, use_pkg, pkg, loader,
		load_mask, &SDL_DisplayFormatAlpha);
}

error_t cpymo_assetloader_load_bg_pixels(
	void ** px, int * w, int * h, 
	cpymo_str name, const cpymo_assetloader * loader)
{ return CPYMO_ERR_UNSUPPORTED; }

error_t cpymo_assetloader_load_bg_image(
	cpymo_backend_image * img, int * w, int * h, 
	cpymo_str name, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_image_with_mask_ex(
		img, w, h, name, "bg", loader->game_config->bgformat,
		NULL, loader->use_pkg_bg, &loader->pkg_bg, loader,
		false, &SDL_DisplayFormat);
}

error_t cpymo_assetloader_load_system_masktrans(
	cpymo_backend_masktrans *out, cpymo_str name, 
	const cpymo_assetloader *loader)
{ 
	char *filename;
	error_t err = cpymo_assetloader_get_fs_path(
		&filename,
		name,
		"system",
		"png",
		loader);		
	CPYMO_THROW(err);

	SDL_Surface *mask = IMG_Load(filename);
	free(filename);
	if (mask == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	Uint8 *mask_surface = (Uint8 *)malloc(
		engine.gameconfig.imagesize_w * engine.gameconfig.imagesize_h);
	if (mask_surface == NULL) {
		SDL_FreeSurface(mask);
		return CPYMO_ERR_OUT_OF_MEM;
	}

	for (Uint16 y = 0; y < engine.gameconfig.imagesize_h; ++y) {
		for (Uint16 x = 0; x < engine.gameconfig.imagesize_w; ++x) {
			float xf = (float)x / (float)engine.gameconfig.imagesize_w;
			float yf = (float)y / (float)engine.gameconfig.imagesize_h;

			int xm = (int)(xf * (mask->w - 1));
			int ym = (int)(yf * (mask->h - 1));
			cpymo_color col = getpixel(mask, xm, ym);
			mask_surface[y * engine.gameconfig.imagesize_w + x] = col.r;
		}
	}

	SDL_FreeSurface(mask);
	*out = (cpymo_backend_masktrans)mask_surface;
	
	return CPYMO_ERR_SUCC;
}

#endif


