#ifndef DISABLE_MOVIE
#include <cpymo_backend_movie.h>
#include <SDL/SDL.h>
#include <libswscale/swscale.h>

extern SDL_Surface *framebuffer;

enum cpymo_backend_movie_how_to_play cpymo_backend_movie_how_to_play()
{ return cpymo_backend_movie_how_to_play_send_surface; }

static SDL_Overlay *overlay = NULL;
static struct SwsContext *sws = NULL;
error_t cpymo_backend_movie_init_surface(
    size_t width, size_t height, enum cpymo_backend_movie_format format)
{
    overlay = SDL_CreateYUVOverlay((int)width, (int)height, SDL_YV12_OVERLAY, framebuffer);
    if (overlay == NULL) {
        return CPYMO_ERR_UNSUPPORTED;
    }

    int pix_format;
    switch (format) {
    case cpymo_backend_movie_format_yuv420p16:
        pix_format = AV_PIX_FMT_YUV420P16;
        break;
    case cpymo_backend_movie_format_yuv420p:
        pix_format = AV_PIX_FMT_YUV420P;
        break;
    case cpymo_backend_movie_format_yuv422p16:
        pix_format = AV_PIX_FMT_YUV422P16;
        break;
    case cpymo_backend_movie_format_yuv422p:
        pix_format = AV_PIX_FMT_YUV422P;
        break;
    case cpymo_backend_movie_format_yuyv422:
        pix_format = AV_PIX_FMT_YUYV422;
        break;
    };

    sws = sws_getContext(
        width, height, pix_format,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_FAST_BILINEAR,
        NULL,
        NULL,
        NULL
    );
    if (sws == NULL) {
        SDL_FreeYUVOverlay(overlay);
        overlay = NULL;
        return CPYMO_ERR_UNSUPPORTED;
    }

    return CPYMO_ERR_SUCC;
}

void cpymo_backend_movie_free_surface()
{
    SDL_FreeYUVOverlay(overlay);
    overlay = NULL;

    sws_freeContext(sws);
    sws = NULL;
}

void cpymo_backend_movie_update_yuv_surface(
	const void *y, size_t y_pitch,
	const void *u, size_t u_pitch,
	const void *v, size_t v_pitch)
{
    if (SDL_LockYUVOverlay(overlay) == -1) return;

    const uint8_t *src[] = {
        (const uint8_t *)y,
        (const uint8_t *)u,
        (const uint8_t *)v
    };

    int src_linesize[] = {
        (int)y_pitch,
        (int)u_pitch,
        (int)v_pitch
    };

    uint8_t *data[] = { 
        overlay->pixels[0],
        overlay->pixels[2],
        overlay->pixels[1]
    };

    int strides[] = {
        overlay->pitches[0],
        overlay->pitches[2],
        overlay->pitches[1]
    };

    sws_scale(sws, src, src_linesize, 0, overlay->h, data, strides);

    SDL_UnlockYUVOverlay(overlay);
}

void cpymo_backend_movie_update_yuyv_surface(const void *p, size_t pitch)
{
    if (SDL_LockYUVOverlay(overlay) == -1) return;

    int src_linesize = (int)pitch;
    int dst_linesize = (int)overlay->pitches[0];

    sws_scale(
        sws, 
        (const uint8_t **)&p, &src_linesize, 0, overlay->h, 
        (uint8_t **)&overlay->pixels[0], &dst_linesize);

    SDL_UnlockYUVOverlay(overlay);
}

void cpymo_backend_movie_draw_surface()
{
    SDL_Rect clip_rect;
    SDL_GetClipRect(framebuffer, &clip_rect);

    float video_ratio = (float)overlay->w / (float)overlay->h;
    float video_height = (float)clip_rect.h;
    float video_width = video_height * video_ratio;
    if (video_width > clip_rect.w) {
        video_width = clip_rect.w;
        video_height = video_width / video_ratio;
    }

    SDL_Rect video_rect;
    video_rect.x = (clip_rect.w - video_width) / 2;
    video_rect.y = (clip_rect.h - video_height) / 2;
    video_rect.w = video_width;
    video_rect.h = video_height;

    SDL_DisplayYUVOverlay(overlay, &video_rect);
}

#endif