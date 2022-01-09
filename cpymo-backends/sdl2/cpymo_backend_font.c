#include <cpymo_error.h>
#include <cpymo_utils.h>
#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

static stbtt_fontinfo font;
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
	free(ttf_buffer);
	ttf_buffer = NULL;
}

error_t cpymo_backend_font_init(const char *gamedir)
{
	char *path = (char *)malloc(strlen(gamedir) + 24);
	if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;
	sprintf(path, "%s/system/default.ttf", gamedir);
	error_t err = cpymo_backend_font_try_load_font(path);
	free(path);
	if (err == CPYMO_ERR_SUCC) return CPYMO_ERR_SUCC;

#ifdef WIN32
	const char *windir = getenv("windir");
	path = (char *)malloc(strlen(windir) + 16);
	if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;

	#define TRY_LOAD(NAME) \
		sprintf(path, "%s/fonts/%s", windir, NAME); \
		err = cpymo_backend_font_try_load_font(path); \
		if (err == CPYMO_ERR_SUCC) { \
			free(path); \
			return CPYMO_ERR_SUCC; \
		}

	TRY_LOAD("msyh.ttc");
	TRY_LOAD("msyhbd.ttc");
	TRY_LOAD("msyhl.ttc");
	TRY_LOAD("msyi.ttf");
	TRY_LOAD("meiryo.ttc");
	TRY_LOAD("meiryob.ttc");
	TRY_LOAD("mingliub.ttc");
	TRY_LOAD("msgothic.ttc");
	TRY_LOAD("msjh.ttc");
	TRY_LOAD("msjhbd.ttc");
	TRY_LOAD("msjhl.ttc");
	TRY_LOAD("msmincho.ttc");
	TRY_LOAD("simhei.ttf");
	TRY_LOAD("simkai.ttf");
	TRY_LOAD("simsun.ttc");
	TRY_LOAD("simsunb.ttf");
	TRY_LOAD("simfang.ttf");
	TRY_LOAD("SIMLI.ttf");

	free(path);
#endif


	return CPYMO_ERR_NOT_FOUND;
}
