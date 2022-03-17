#include <cpymo_error.h>
#include <cpymo_utils.h>
#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

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
	}

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
		"/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf"
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


	return CPYMO_ERR_NOT_FOUND;
}
