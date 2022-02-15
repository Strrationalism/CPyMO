#ifndef INCLUDE_CPYMO_PACKAGE
#define INCLUDE_CPYMO_PACKAGE

#include <stdio.h>
#include <stdint.h>
#include "cpymo_parser.h"
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

error_t cpymo_package_open(cpymo_package *out_package, const char *path);
void cpymo_package_close(cpymo_package *package);
error_t cpymo_package_find(cpymo_package_index *out_index, const cpymo_package *package, const char* filename);
error_t cpymo_package_read_file(char *out_buffer, const cpymo_package *package, const cpymo_package_index *index);


typedef struct {
	size_t file_offset;
	size_t file_length;
	size_t current;
	FILE *stream;
} cpymo_package_stream_reader;

cpymo_package_stream_reader cpymo_package_stream_reader_create(
	const cpymo_package *package, 
	const cpymo_package_index *index);

error_t cpymo_package_stream_reader_find_create(
	cpymo_package_stream_reader *r,
	const cpymo_package *package,
	cpymo_parser_stream_span name);

error_t cpymo_package_stream_reader_seek(
	size_t seek,
	cpymo_package_stream_reader *r);

size_t cpymo_package_stream_reader_read(
	char *dst_buf,
	size_t dst_buf_size,
	cpymo_package_stream_reader *r);

#endif
