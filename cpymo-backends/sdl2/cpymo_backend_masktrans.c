#include <cpymo_backend_masktrans.h>

error_t cpymo_backend_masktrans_create(cpymo_backend_masktrans *out, void *mask_singlechannel_moveinto, int w, int h)
{
	return CPYMO_ERR_UNSUPPORTED;
}

void cpymo_backend_masktrans_free(cpymo_backend_masktrans mt) {}

void cpymo_backend_mask_trans_draw_fullscreen_fadeout(cpymo_backend_masktrans mt, float t) {}
void cpymo_backend_mask_trans_draw_fullscreen_fadein(cpymo_backend_masktrans mt, float t) {}

