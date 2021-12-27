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

int cpymo_utils_clamp(int v, int minv, int maxv)
{
	if (v > maxv) return maxv;
	else if (v < minv) return minv;
	else return v;
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