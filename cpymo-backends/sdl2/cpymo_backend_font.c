#include <cpymo_error.h>
#include <cpymo_utils.h>
#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

static stbtt_fontinfo font;
static unsigned char *ttf_buffer = NULL;


static error_t cpymo_backend_font_try_load_font(const char *path, bool isTTC)
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
	free(ttf_buffer);
	ttf_buffer = NULL;
}

error_t cpymo_backend_font_init(const char *gamedir)
{
	char *path = (char *)malloc(strlen(gamedir) + 24);
	if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;
	sprintf(path, "%s/system/default.ttf", gamedir);
	error_t err = cpymo_backend_font_try_load_font(path, false);
	free(path);
	if (err == CPYMO_ERR_SUCC) return CPYMO_ERR_SUCC;

#ifdef WIN32
	const char *windir = getenv("windir");
	path = (char *)malloc(strlen(windir) + 16);
	if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;

	#define TRY_LOAD(NAME, ISTTC) \
		sprintf(path, "%s/fonts/%s", windir, NAME); \
		err = cpymo_backend_font_try_load_font(path, ISTTC); \
		if (err == CPYMO_ERR_SUCC) { \
			free(path); \
			return CPYMO_ERR_SUCC; \
		}

	TRY_LOAD("msyh.ttc", true);
	TRY_LOAD("msyhbd.ttc", true);
	TRY_LOAD("msyhl.ttc", true);
	TRY_LOAD("msyi.ttf", false);
	TRY_LOAD("meiryo.ttc", true);
	TRY_LOAD("meiryob.ttc", true);
	TRY_LOAD("mingliub.ttc", true);
	TRY_LOAD("msgothic.ttc", true);
	TRY_LOAD("msjh.ttc", true);
	TRY_LOAD("msjhbd.ttc", true);
	TRY_LOAD("msjhl.ttc", true);
	TRY_LOAD("msmincho.ttc", true);
	TRY_LOAD("simhei.ttf", false);
	TRY_LOAD("simkai.ttf", false);
	TRY_LOAD("simsun.ttc", true);
	TRY_LOAD("simsunb.ttf", false);
	TRY_LOAD("simfang.ttf", false);
	TRY_LOAD("SIMLI.ttf", false);

	free(path);
#endif


	return CPYMO_ERR_NOT_FOUND;
}
