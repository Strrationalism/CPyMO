#include <cpymo_backend_masktrans.h>
#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>
#include <cpymo_backend_image.h>
#include <stdlib.h>

typedef struct {
	C3D_Tex tex;
	void *mask;
	int w, h;
	int pw, ph;
} cpymo_backend_masktrans_i;

int pad_tex_size(int s);

error_t cpymo_backend_masktrans_create(cpymo_backend_masktrans *out, void *mask_singlechannel_moveinto, int w, int h)
{
	int pad_width = pad_tex_size(w);
    int pad_height = pad_tex_size(h);

	int pad = pad_width > pad_height ? pad_width : pad_height;
	pad_width = pad;
	pad_height = pad;

	if (pad_width > 1024 || pad_height > 1024 || pad_width < 0 || pad_height < 0)
		return CPYMO_ERR_UNSUPPORTED;

	cpymo_backend_masktrans_i *t = 
		(cpymo_backend_masktrans_i *)malloc(sizeof(cpymo_backend_masktrans_i));

	if (t == NULL) return CPYMO_ERR_OUT_OF_MEM;

	t->mask = mask_singlechannel_moveinto;
	t->w = w;
	t->h = h;
	t->pw = pad_width;
	t->ph = pad_height;

	if (!C3D_TexInit(&t->tex, (u16)pad_width, (u16)pad_height, GPU_A8)) {
		free(t);
		return CPYMO_ERR_UNKNOWN;
	}

	printf("[Load Masktrans] W: %d, H: %d, PW: %d, PH: %d", 
		t->w, t->h, t->pw, t->ph);

	*out = t;

	return CPYMO_ERR_SUCC;
}

void cpymo_backend_masktrans_free(cpymo_backend_masktrans m)
{
	cpymo_backend_masktrans_i *t = (cpymo_backend_masktrans_i *)m;
	free(t->mask);
	C3D_TexDelete(&t->tex);
	free(t);
}

extern const float game_width, game_height;

float offset_3d(enum cpymo_backend_image_draw_type type);
void trans_pos(float *x, float *y);
void trans_size(float *w, float *h);

void cpymo_backend_masktrans_draw(cpymo_backend_masktrans m, float progression, bool is_fade_in)
{
	if (!is_fade_in) progression = 1.0f - progression;
	const float radius = 0.25f;
	progression = progression * (1.0f + 2 * radius) - radius;

	const float t_top = progression + radius;
	const float t_bottom = progression - radius;

	cpymo_backend_masktrans_i *t = (cpymo_backend_masktrans_i *)m;

    for(u32 y = 0; y < t->h; y++) {
        for(u32 x = 0; x < t->w; x++) {
            u32 ix = t->pw - x - 1;
            u32 dstPos = ((((ix >> 3) * (t->ph >> 3) + (y >> 3)) << 6) + ((y & 1) | ((ix & 1) << 1) | ((y & 2) << 1) | ((ix & 2) << 2) | ((y & 4) << 2) | ((ix & 4) << 3)));
            u8 *srcPos = (u8*)t->mask + (y * t->w + x);

			float mask = (float)*srcPos / 255.0f;
			if(is_fade_in) mask = 1 - mask;

			if (mask > t_top) mask = 1.0f;
			else if (mask < t_bottom) mask = 0.0f;
			else mask = (mask - t_bottom) / (2 * radius);

			((u8 *)t->tex.data)[dstPos] = (u8)(mask * 255.0f);
        }
    }

	C3D_TexFlush(&t->tex);
    
	Tex3DS_SubTexture sub;
    sub.width = (u16)t->w;
    sub.height = (u16)t->h;
    sub.left = 0;
    sub.right = (float)t->w / (float)t->tex.width;
    sub.top = 0;
    sub.bottom = (float)t->h / (float)t->tex.height;

	C2D_Image cimg;
    cimg.subtex = &sub;
    cimg.tex = &t->tex;

	float x = 0, y = 0;
	float w = game_width, h = game_height;
	trans_pos(&x, &y);
	trans_size(&w, &h);

	x += offset_3d(cpymo_backend_image_draw_type_bg);

	C2D_DrawParams p;
    p.angle = 0;
    p.center.x = 0;
    p.center.y = 0;
    p.depth = 0;
    p.pos.x = x;
    p.pos.y = y;
    p.pos.w = w;
    p.pos.h = h;

	C2D_DrawImage(cimg, &p, NULL);
}