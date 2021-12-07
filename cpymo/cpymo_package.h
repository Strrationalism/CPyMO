#ifndef INCLUDE_PYMO_PACKAGE
#define INCLUDE_PYMO_PACKAGE

#include <stdio.h>
#include <stdint.h>
#include "cpymo_error.h"

typedef struct {
	char file_name[32];
	uint32_t file_offset;
	uint32_t file_length;
} cpymo_package_index;

typedef struct {
	uint32_t file_count;
	cpymo_package_index *files;
	FILE *stream;
} cpymo_package;

extern error_t cpymo_package_open(cpymo_package *out_package, const char *path);
extern void cpymo_package_close(cpymo_package *package);
extern error_t cpymo_package_find(cpymo_package_index *out_index, const cpymo_package *package, const char* filename);
extern error_t cpymo_package_read_file(char *out_buffer, const cpymo_package *package, const cpymo_package_index *index);

#endif
