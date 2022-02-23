#include "cpymo_rmenu.h"
#include "cpymo_engine.h"
#include <assert.h>

typedef struct {
	cpymo_backend_image bg;
	int bg_w, bg_h;

	cpymo_select_img menu;
	cpymo_wait menu_waiter;

	bool alive;
} cpymo_rmenu;

static error_t cpymo_rmenu_update(cpymo_engine *e, void *ui_data, float dt)
{
	cpymo_rmenu *r = (cpymo_rmenu *)ui_data;

	if (r->alive) {
		error_t err = cpymo_wait_update(&r->menu_waiter, e, dt);
		CPYMO_THROW(err);

		if (r->alive) {
			err = cpymo_select_img_update(e, &r->menu, dt);
			CPYMO_THROW(err);
		}
	}

	return CPYMO_ERR_SUCC;
}

static void cpymo_rmenu_draw(const cpymo_engine *e, const void *ui_data)
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
			cpymo_backend_image_draw_type_ui_bg);
	}
	else {
		cpymo_backend_image_fill_rects(
			bg_xywh, 1, cpymo_color_black, 0.75f, 
			cpymo_backend_image_draw_type_ui_bg);
	}

	cpymo_select_img_draw(&r->menu, e->gameconfig.imagesize_w, e->gameconfig.imagesize_h, false);
}

static void cpymo_rmenu_delete(cpymo_engine *e, void *ui_data)
{
	cpymo_rmenu *r = (cpymo_rmenu *)ui_data;
	r->alive = false;

	if (r->bg) cpymo_backend_image_free(r->bg);

	cpymo_select_img_free(&r->menu);
}

static error_t cpymo_rmenu_ok(cpymo_engine *e, int sel, uint64_t hash, bool _)
{
	switch (sel) {
	case 0: cpymo_ui_exit(e); break;
	case 1: cpymo_ui_exit(e); break;
	case 2: cpymo_backlog_ui_enter(e); break;
	case 3: cpymo_ui_exit(e); break;
	case 4: {
		char *gamedir = (char *)malloc(strlen(e->assetloader.gamedir) + 1);
		if (gamedir == NULL) return CPYMO_ERR_OUT_OF_MEM;
		
		strcpy(gamedir, e->assetloader.gamedir);
		cpymo_engine_free(e);
		error_t err = cpymo_engine_init(e, gamedir);
		free(gamedir);

		CPYMO_THROW(err);
		break;
	}
	case 5: return CPYMO_ERR_NO_MORE_CONTENT;
	case 6: cpymo_ui_exit(e); break;
	default:
		assert(false);
	}

	return CPYMO_ERR_SUCC;
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

	rmenu->alive = true;

	cpymo_select_img_init(&rmenu->menu);
	cpymo_wait_reset(&rmenu->menu_waiter);

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

	err = cpymo_select_img_configuare_begin(
		&rmenu->menu,
		7,
		cpymo_parser_stream_span_pure(""),
		&e->assetloader,
		&e->gameconfig);
	if (err != CPYMO_ERR_SUCC) {
		cpymo_ui_exit(e);
		return err;
	}

	#define RMENU_ITEM(_, TEXT, ENABLED) \
		err = cpymo_select_img_configuare_select_text( \
			&rmenu->menu, \
			&e->assetloader, \
			&e->gameconfig, \
			&e->flags, \
			cpymo_parser_stream_span_pure(TEXT), \
			ENABLED, \
			cpymo_select_img_selection_nohint, \
			0); \
		if (err != CPYMO_ERR_SUCC) { \
			cpymo_ui_exit(e); \
			return err; \
		}

	cpymo_select_img_configuare_set_ok_callback(&rmenu->menu, &cpymo_rmenu_ok);

	RMENU_ITEM(0, "保存存档", false);
	RMENU_ITEM(1, "读取存档", false);
	RMENU_ITEM(2, "对话记录", true);
	RMENU_ITEM(3, "游戏设置", false)
	RMENU_ITEM(4, "重启游戏", true);
	RMENU_ITEM(5, "退出游戏", true);
	RMENU_ITEM(6, "返回游戏", true);

	float xywh[4] = {
		((float)e->gameconfig.imagesize_w - (float)rmenu->bg_w) / 2,
		((float)e->gameconfig.imagesize_h - (float)rmenu->bg_h) / 2,
		(float)rmenu->bg_w,
		(float)rmenu->bg_h,
	};


	cpymo_select_img_configuare_end_select_text(
		&rmenu->menu,
		&rmenu->menu_waiter,
		e,
		xywh[0],
		xywh[1],
		xywh[0] + xywh[2],
		xywh[1] + xywh[3],
		cpymo_color_white,
		0,
		false);

	rmenu->menu.draw_type = cpymo_backend_image_draw_type_ui_element;

	return CPYMO_ERR_SUCC;
}
