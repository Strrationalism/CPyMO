#include "cpymo_rmenu.h"
#include "cpymo_engine.h"

typedef struct {
	cpymo_backend_image bg;
	int bg_w, bg_h;
} cpymo_rmenu;

error_t cpymo_rmenu_update(cpymo_engine *e, void *ui_data, float dt)
{
	if (CPYMO_INPUT_JUST_PRESSED(e, cancel)) {
		cpymo_ui_exit(e);
		return CPYMO_ERR_SUCC;
	}

	return CPYMO_ERR_SUCC;
}

void cpymo_rmenu_draw(const cpymo_engine *e, const void *ui_data)
{
	const cpymo_rmenu *r = (const cpymo_rmenu *)ui_data;

	cpymo_bg_draw(e);
	cpymo_scroll_draw(&e->scroll);

	float bg_xywh[4] = {
		((float)e->gameconfig.imagesize_w - (float)r->bg_w) / 2,
		((float)e->gameconfig.imagesize_h - (float)r->bg_h) / 2,
		(float)r->bg_w,
		(float)r->bg_h,
	};

	if (r->bg) {
		cpymo_backend_image_draw(
			bg_xywh[0],
			bg_xywh[1],
			bg_xywh[2],
			bg_xywh[3],
			r->bg,
			0,
			0,
			r->bg_w,
			r->bg_h,
			1.0f,
			cpymo_backend_image_draw_type_uielement);
	}
	else {
		cpymo_backend_image_fill_rects(
			bg_xywh, 1, cpymo_color_black, 0.75f, 
			cpymo_backend_image_draw_type_uielement);
	}
}

void cpymo_rmenu_delete(cpymo_engine *e, void *ui_data)
{
	cpymo_rmenu *r = (cpymo_rmenu *)ui_data;
	if (r->bg) cpymo_backend_image_free(r->bg);
}

error_t cpymo_rmenu_enter(cpymo_engine *e)
{
	cpymo_rmenu *rmenu = NULL;
	error_t err = cpymo_ui_enter(
		(void **)&rmenu,
		e,
		sizeof(cpymo_rmenu),
		&cpymo_rmenu_update,
		&cpymo_rmenu_draw,
		&cpymo_rmenu_delete);
	CPYMO_THROW(err);

	rmenu->bg = NULL;
	err = cpymo_assetloader_load_system_image(
		&rmenu->bg,
		&rmenu->bg_w,
		&rmenu->bg_h,
		cpymo_parser_stream_span_pure("menu"),
		"png",
		&e->assetloader,
		cpymo_gameconfig_is_symbian(&e->gameconfig));

	if (err != CPYMO_ERR_SUCC) {
		rmenu->bg = NULL;

		if (strcmp(e->gameconfig.platform, "s60v3") == 0) {
			rmenu->bg_w = 240;
			rmenu->bg_h = 144;
		}
		else if (strcmp(e->gameconfig.platform, "s60v5") == 0) {
			rmenu->bg_w = 360;
			rmenu->bg_h = 216;
		}
		else {
			rmenu->bg_w = (int)((float)e->gameconfig.imagesize_w * 0.75f);
			rmenu->bg_h = (int)((float)e->gameconfig.imagesize_h * 0.6f);
		}
	}

	return CPYMO_ERR_SUCC;
}
