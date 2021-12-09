#ifndef INCLUDE_CPYMO_UTILS
#define INCLUDE_CPYMO_UTILS

#include "cpymo_error.h"
#include <stddef.h>

error_t cpymo_utils_loadfile(const char *path, char **outbuf, size_t *len);
int cpymo_utils_clamp(int v, int minv, int maxv);

void cpymo_utils_replace_str_newline_n(char *str);

#endif
