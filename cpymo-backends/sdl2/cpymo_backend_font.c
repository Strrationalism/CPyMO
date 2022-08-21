#include "cpymo_prelude.h"
#ifndef DISABLE_STB_TRUETYPE

#include <cpymo_error.h>
#include <cpymo_utils.h>
#include <cpymo_parser.h>
#include <stdio.h>

#ifdef __ANDROID__
#include "cpymo_import_sdl2.h"
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#ifdef __UWP__
#include <malloc.h>
#endif

#include "posix_win32.h"

#ifdef __SWITCH__
#include <switch.h>
#endif


stbtt_fontinfo font;
static unsigned char *ttf_buffer = NULL;


static error_t cpymo_backend_font_try_load_font(const char *path)
{
	size_t buf_size = 0;
	
	error_t err = cpymo_utils_loadfile(path, (char **)&ttf_buffer, &buf_size);
	CPYMO_THROW(err);

	if (stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0)) == 0) {
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	printf("[Info] Load font %s.\n", path);

	return CPYMO_ERR_SUCC;
}

void cpymo_backend_font_free()
{
	if (ttf_buffer) free(ttf_buffer);
	ttf_buffer = NULL;
}

error_t cpymo_backend_font_init(const char *gamedir)
{
	char *path = (char *)alloca((gamedir ? strlen(gamedir) : 0) + 24);
	error_t err = CPYMO_ERR_SUCC;

	if (gamedir) {
		if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;
		sprintf(path, "%s/system/default.ttf", gamedir);
		err = cpymo_backend_font_try_load_font(path);
		if (err == CPYMO_ERR_SUCC) return CPYMO_ERR_SUCC;

		sprintf(path, "%s/system/default.otf", gamedir);
		err = cpymo_backend_font_try_load_font(path);
		if (err == CPYMO_ERR_SUCC) return CPYMO_ERR_SUCC;
	}

#ifdef GAME_SELECTOR_DIR
	err = cpymo_backend_font_try_load_font(GAME_SELECTOR_DIR "/default.ttf");
	if (err == CPYMO_ERR_SUCC) return err;

	err = cpymo_backend_font_try_load_font(GAME_SELECTOR_DIR "/default.otf");
	if (err == CPYMO_ERR_SUCC) return err;
#endif
	
#ifdef GAME_SELECTOR_DIR_2
	err = cpymo_backend_font_try_load_font(GAME_SELECTOR_DIR_2 "/default.ttf");
	if (err == CPYMO_ERR_SUCC) return err;

	err = cpymo_backend_font_try_load_font(GAME_SELECTOR_DIR_2 "/default.otf");
	if (err == CPYMO_ERR_SUCC) return err;
#endif

#ifdef _WIN32
	const char *windir = getenv("windir");
	path = (char *)alloca(strlen(windir) + 32);
	if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;

	const char *fonts[] = {
		"msyh.ttc",
		"msyhbd.ttc",
		"msyhl.ttc",
		"msyi.ttf",
		"meiryo.ttc",
		"meiryob.ttc",
		"mingliub.ttc",
		"msgothic.ttc",
		"msjh.ttc",
		"msjhbd.ttc",
		"msjhl.ttc",
		"msmincho.ttc",
		"simhei.ttf",
		"simkai.ttf",
		"simsun.ttc"
		"simsunb.ttf",
		"simfang.ttf",
		"SIMLI.ttf"
	};

	for (size_t i = 0; i < sizeof(fonts) / sizeof(fonts[0]); ++i) {
		sprintf(path, "%s/fonts/%s", windir, fonts[i]);
		err = cpymo_backend_font_try_load_font(path);
		if (err == CPYMO_ERR_SUCC) return CPYMO_ERR_SUCC;
	}
#endif

#ifdef __LINUX__
	const char *fonts[] = {
		// Ubuntu
		"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
		"/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc",
		"/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc",
		"/usr/share/fonts/opentype/noto/NotoSerifCJK-Bold.ttc",

		// Deepin
		"/usr/share/fonts/truetype/unifont/unifont.ttf",
	};

	for (size_t i = 0; i < sizeof(fonts) / sizeof(fonts[0]); ++i) 
		if (cpymo_backend_font_try_load_font(fonts[i]) == CPYMO_ERR_SUCC)
			return CPYMO_ERR_SUCC;
#endif

#ifdef __APPLE__
	const char *fonts[] = {
		"/System/Library/Fonts/STHeiti Medium.ttc",
		"/System/Library/Fonts/STHeiti Light.ttc"
	};

	for (size_t i = 0; i < sizeof(fonts) / sizeof(fonts[0]); ++i)
		if (cpymo_backend_font_try_load_font(fonts[i]) == CPYMO_ERR_SUCC)
			return CPYMO_ERR_SUCC;
#endif

#ifdef __SWITCH__
	Result r = plInitialize(PlServiceType_User);
	if (R_FAILED(r)) return CPYMO_ERR_NOT_FOUND;

	PlFontData font_infos[PlSharedFontType_Total];
	s32 total_fonts;
	r = plGetSharedFont(SetLanguage_ZHCN, font_infos, PlSharedFontType_Total, &total_fonts);
	if (R_FAILED(r)) return CPYMO_ERR_NOT_FOUND;
	for (s32 i = 1; i < total_fonts; ++i) {		// Skip 0 font, font 0 is shit.
		const void *address = font_infos[i].address;
		if (stbtt_InitFont(&font, address, stbtt_GetFontOffsetForIndex(address, 0)) == 0) {
			continue;
		}

		return CPYMO_ERR_SUCC;
	}
	plExit();
#endif

#ifdef __ANDROID__
	SDL_AndroidShowToast("Can not find /sdcard/pymogames/default.ttf", 1, -1, 0, 0);
#endif

	return CPYMO_ERR_CAN_NOT_OPEN_FILE;
}

#ifdef __SWITCH__
#define TEXT_LINE_Y_OFFSET 12
#else
#define TEXT_LINE_Y_OFFSET 0
#endif


void cpymo_backend_font_render(void *out_or_null, int *w, int *h, cpymo_parser_stream_span text, float scale, float baseline) {
	float xpos = 0;

	int width = 0, height = 0;
	float y_base = 0;
	while (text.len > 0) {
		uint32_t codepoint = cpymo_parser_stream_span_utf8_try_head_to_utf32(&text);
		int x0, y0, x1, y1;

		int advance_width, lsb;
		float x_shift = xpos - (float)floor(xpos);
		stbtt_GetCodepointHMetrics(&font, (int)codepoint, &advance_width, &lsb);

		if (codepoint == '\n') {
			xpos = 0;

			y_base += baseline + TEXT_LINE_Y_OFFSET;

			continue;
		}

		stbtt_GetCodepointBitmapBoxSubpixel(&font, (int)codepoint, scale, scale, x_shift, 0, &x0, &y0, &x1, &y1);
		if (out_or_null)
			stbtt_MakeCodepointBitmapSubpixel(
				&font,
				(unsigned char *)out_or_null + (int)xpos + x0 + (int)(baseline + y0 + y_base) * *w,
				x1 - x0, y1 - y0, *w, scale, scale, x_shift, 0, (int)codepoint);

		xpos += (advance_width * scale);

		cpymo_parser_stream_span text2 = text;
		uint32_t next_char = cpymo_parser_stream_span_utf8_try_head_to_utf32(&text2);
		if (next_char) {
			xpos += scale * stbtt_GetCodepointKernAdvance(&font, codepoint, next_char);
		}

		int new_width = (int)ceil(xpos);
		if (new_width > width) width = new_width;

		int new_height = (int)((y1 - y0) + baseline + y_base);
		if (new_height > height) height = new_height;
	}

	*w = width;
	*h = height;
}

#endif
