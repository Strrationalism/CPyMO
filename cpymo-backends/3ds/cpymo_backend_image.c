#include <cpymo_backend_image.h>
#include <cpymo_engine.h>
#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <assert.h>
#include <cpymo_utils.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

static float offset_3d(enum cpymo_backend_image_draw_type type)
{
    switch(type) {
        case cpymo_backend_image_draw_type_bg: return -10.0f;
        case cpymo_backend_image_draw_type_chara: return -5.0f;
        case cpymo_backend_image_draw_type_sel_img: return 5.0f;
        default: return 0.0f;
    }
}

const extern float render_3d_offset;

static float game_width, game_height;
static float viewport_width, viewport_height;
static float offset_x, offset_y;

const extern bool fill_screen;

void cpymo_backend_image_init(float game_w, float game_h)
{
    game_width = game_w;
    game_height = game_h;

    float ratio_w = game_w / 400;
    float ratio_h = game_h / 240;

    if(ratio_w > ratio_h) {
        viewport_width = 400;
        viewport_height = game_h / ratio_w;
    } 
    else {
        viewport_width = game_w / ratio_h;
        viewport_height = 240;
    }

    offset_x = 400 / 2 - viewport_width / 2;
    offset_y = 240 / 2 - viewport_height / 2;
}

static void trans_size(float *w, float *h) {
    if(fill_screen) {
        *w = *w / game_width * (400 + 20);
        *h = *h / game_height * 240;
    }
    else {
        *w = *w / game_width * viewport_width;
        *h = *h / game_height * viewport_height;
    }
}

static void trans_pos(float *x, float *y) {
    if(fill_screen) {
        *x = *x / game_width * (400 + 20) - 10;
        *y = *y / game_height * 240;
    }
    else {
        *x = *x / game_width * viewport_width + offset_x;
        *y = *y / game_height * viewport_height + offset_y;
    }
}

void cpymo_backend_image_fill_screen_edges()
{
    if(!fill_screen) {
        const u32 col = C2D_Color32(0, 0, 0, 255);

        if(offset_x > 0.5f) {
            C2D_DrawRectSolid(-10 * render_3d_offset - 10, 0, 0, offset_x + 10, 240, col);
            C2D_DrawRectSolid(-10 * render_3d_offset + offset_x + viewport_width, 0, 0, offset_x + 10, 240, col);
        }

        if(offset_y > 0.5f) {
            C2D_DrawRectSolid(0, 0, 0, 400, offset_y, col);
            C2D_DrawRectSolid(0, offset_y + viewport_height, 0, 400, offset_y, col);
        }
    }
}

void cpymo_backend_image_fill_rects(
	const float *xywh, size_t count,
	cpymo_color color, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{
    for(size_t i = 0; i < count; ++i) {
        float x = xywh[i * 4];
        float y = xywh[i * 4 + 1];
        float w = xywh[i * 4 + 2];
        float h = xywh[i * 4 + 3];

        trans_pos(&x, &y);
        trans_size(&w, &h);

        x += offset_3d(draw_type) * render_3d_offset;
        C2D_DrawRectSolid(x, y, 0.0, w, h, C2D_Color32(color.r, color.g, color.b, (u8)(255 * alpha)));
    }
}

int pad_tex_size(int s) 
{
    int r = 8;
    while(s > r) r *= 2;

    if(r > 1024) return -1;
    else return r;
}

typedef struct {
    C3D_Tex tex;
    u16 real_width;
    u16 real_height;
    u16 pad_width;
    u16 pad_height;
    float scale;
} cpymo_backend_image_3ds;

error_t cpymo_backend_image_load(
    cpymo_backend_image *out_image, 
    void *pixels_moveintoimage, 
    int width,
    int height, 
    enum cpymo_backend_image_format fmt)
{
    int pad_width = pad_tex_size(width);
    int pad_height = pad_tex_size(height);
    float scale = 1.0f;

    int channels;
    GPU_TEXCOLOR tex_fmt;
    switch(fmt) {
    case cpymo_backend_image_format_r: tex_fmt = GPU_A8; channels = 1; break;
    case cpymo_backend_image_format_rgb: tex_fmt = GPU_RGB8; channels = 3; break;
    case cpymo_backend_image_format_rgba: tex_fmt = GPU_RGBA8; channels = 4; break;
    default: assert(false);
    };

    if(width > 1024 || height > 1024) {
        
        int max_edge = width > height ? width : height;
        scale = 512.0f / max_edge;
        assert(scale > 0 && scale <= 1.0f);

        int new_width = cpymo_utils_clamp((int)(scale * width), 8, 1024);
        int new_height = cpymo_utils_clamp((int)(scale * height), 8, 1024);

        printf("[Load Texture] Scaling: %d, %d => %d, %d\n", width ,height ,new_width, new_height);

        void *new_px = malloc(new_width * new_height * channels);
        if(new_px == NULL) return CPYMO_ERR_OUT_OF_MEM;
        
        if(!stbir_resize_uint8(
            pixels_moveintoimage, width, height, width * channels, 
            new_px, new_width, new_height, new_width * channels, channels))
        {
            free(new_px);
            return CPYMO_ERR_UNKNOWN;
        }

        width = new_width;
        height = new_height;
        pad_width = pad_tex_size(width);
        pad_height = pad_tex_size(height);

        free(pixels_moveintoimage);
        pixels_moveintoimage = new_px;
    }

    printf("[Load Texture] C: %d, W: %d, H: %d, PW: %d, PH: %d\n",
    channels, width, height, pad_width, pad_height);

    cpymo_backend_image_3ds *img = (cpymo_backend_image_3ds *)malloc(sizeof(cpymo_backend_image_3ds));
    if(img == NULL) return CPYMO_ERR_OUT_OF_MEM;

    img->real_width = (u16)width;
    img->real_height = (u16)height;
    img->pad_width = (u16)pad_width;
    img->pad_height = (u16)pad_height;
    img->scale = scale;

    if(!C3D_TexInit(&img->tex, (u16)pad_height, (u16)pad_width, tex_fmt)) {
        free(img);
        return CPYMO_ERR_UNKNOWN;
    }

    C3D_TexSetFilter(&img->tex, GPU_LINEAR, GPU_LINEAR);

    memset(img->tex.data, 0, img->tex.size);
    
    for(u32 x = 0; x < width; x++) {
        for(u32 y = 0; y < height; y++) {
            for(int channel = 0; channel < channels; ++channel) {
                u32 ix = pad_width - x - 1;
                u32 dstPos = ((((ix >> 3) * (pad_height >> 3) + (y >> 3)) << 6) + ((y & 1) | ((ix & 1) << 1) | ((y & 2) << 1) | ((ix & 2) << 2) | ((y & 4) << 2) | ((ix & 4) << 3))) * channels;
                u8 *srcPos = (u8*)pixels_moveintoimage + (y * width + x) * channels;

                memcpy(&((u8*)img->tex.data)[dstPos], srcPos, channels);
                for(int c = 0; c < channels; c++) {
                    ((u8 *)img->tex.data)[dstPos + c] = srcPos[channels - 1 - c];
                }
            }
        }
    }

    free(pixels_moveintoimage);
    C3D_TexFlush(&img->tex);

    *out_image = img;
    return CPYMO_ERR_SUCC;
}

void cpymo_backend_image_free(cpymo_backend_image image)
{
    C3D_TexDelete(&((cpymo_backend_image_3ds *)image)->tex);
    free(image);
}

void cpymo_backend_image_draw(
	float dstx, float dsty, float dstw, float dsth,
	cpymo_backend_image src,
	int srcx, int srcy, int srcw, int srch, float alpha,
	enum cpymo_backend_image_draw_type draw_type)
{
    cpymo_backend_image_3ds *img = (cpymo_backend_image_3ds *)src;
    
    Tex3DS_SubTexture sub;
    sub.width = (u16)(srcw * img->scale);
    sub.height = (u16)(srch * img->scale);
    sub.left = img->scale * (float)srcx / (float)img->pad_width;
    sub.right = img->scale * (float)(srcx + srcw) / (float)img->pad_width;
    sub.top = img->scale * (float)srcy / (float)img->pad_height;
    sub.bottom = img->scale * (float)(srcy + srch) / (float)img->pad_height;

    C2D_Image cimg;
    cimg.subtex = &sub;
    cimg.tex = &img->tex;

    trans_pos(&dstx, &dsty);
    trans_size(&dstw, &dsth);

    dstx += offset_3d(draw_type) * render_3d_offset;
    
    C2D_DrawParams p;
    p.angle = 0;
    p.center.x = 0;
    p.center.y = 0;
    p.depth = 0;
    p.pos.x = dstx;
    p.pos.y = dsty;
    p.pos.w = dstw;
    p.pos.h = dsth;

    C2D_ImageTint tint;
    C2D_AlphaImageTint(&tint, alpha);
    C2D_DrawImage(cimg, &p, &tint);
}

error_t cpymo_backend_image_load_with_mask(
	cpymo_backend_image *out_image, 
    void *px_rgbx32_moveinto, 
    void *mask_a8_moveinto, 
    int w, 
    int h)
{
	cpymo_utils_attach_mask_to_rgba(px_rgbx32_moveinto, mask_a8_moveinto, w, h);
	free(mask_a8_moveinto);
	return cpymo_backend_image_load(out_image, px_rgbx32_moveinto, w, h, cpymo_backend_image_format_rgba);
}