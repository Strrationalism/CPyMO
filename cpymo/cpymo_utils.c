#include "cpymo_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

error_t cpymo_utils_loadfile(const char *path, char **outbuf, size_t *len)
{
	assert(*outbuf == NULL);

	FILE *f = fopen(path, "rb");
	if (f == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	fseek(f, 0, SEEK_END);
	*len = ftell(f);
	fseek(f, 0, SEEK_SET);

	*outbuf = (char *)malloc(*len);
	if (*outbuf == NULL) {
		fclose(f);
		return CPYMO_ERR_OUT_OF_MEM;
	}

	if (fread(*outbuf, *len, 1, f) != 1) {
		fclose(f);
		free(*outbuf);
		return CPYMO_ERR_CAN_NOT_OPEN_FILE;
	}

	fclose(f);
	return CPYMO_ERR_SUCC;
}

void cpymo_utils_replace_str_newline_n(char *str)
{
	char prev_char = '?';
	
	while (*str) {
		if (prev_char == '\\' && *str == 'n') {
			*str = '\n';
			*(str - 1) = ' ';
		}

		prev_char = *str;
		str++;
	}
}

bool cpymo_utils_string_equals_ignore_case(const char * str1, const char * str2)
{
	if (*str1 == '\0' && *str2 == '\0') return true;
	else if (*str1 == '\0' || *str2 == '\0') return false;
	else {
		if (toupper(*str1) == toupper(*str2)) return cpymo_utils_string_equals_ignore_case(str1 + 1, str2 + 1);
		else return false;
	}
}

void cpymo_utils_attach_mask_to_rgba(void *rgba_, void *mask_, int w, int h)
{
	char *rgba = (char *)rgba_;
	char *mask = (char *)mask_;

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			rgba[(y * w + x) * 4 + 3] = mask[y * w + x];
		}
	}
}

void cpymo_utils_attach_mask_to_rgba_ex(void * rgba_, int w, int h, void * mask_, int mask_w, int mask_h)
{
	if (w == mask_w && h == mask_h) {
		cpymo_utils_attach_mask_to_rgba(rgba_, mask_, w, h);
		return;
	}
	
	char *rgba = (char *)rgba_;
	char *mask = (char *)mask_;

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			float fx = (float)x / (float)w;
			float fy = (float)y / (float)h;

			int mx = (int)(fx * (float)mask_w);
			int my = (int)(fy * (float)mask_h);
			rgba[(y * w + x) * 4 + 3] = mask[my * mask_w + mx];
		}
	}
}
