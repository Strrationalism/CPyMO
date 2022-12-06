#ifndef INCLUDE_CPYMO_PACKAGE
#define INCLUDE_CPYMO_PACKAGE

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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

#ifdef DEBUG
	bool has_stream_reader;
#endif
} cpymo_package;

error_t cpymo_package_open(cpymo_package *out_package, const char *path);
void cpymo_package_close(cpymo_package *package);
error_t cpymo_package_find(cpymo_package_index *out_index, const cpymo_package *package, cpymo_str filename);
error_t cpymo_package_read_file_from_index(char *out_buffer, const cpymo_package *package, const cpymo_package_index *index);
error_t cpymo_package_read_file(char **out_buffer, size_t *sz, const cpymo_package *package, cpymo_str filename);

error_t cpymo_package_read_image_from_index(
	void **pixels, int *w, int *h, int channels, 
	const cpymo_package *pkg, const cpymo_package_index *index);

error_t cpymo_package_read_image(
	void **pixels, int *w, int *h, int channels,
	const cpymo_package *pkg, cpymo_str filename);

typedef struct {
	size_t file_offset;
	size_t file_length;
	size_t current;
	FILE *stream;

#ifdef DEBUG
	cpymo_package *package;
#ifdef LEAKCHECK
	void *leak_marker;
#endif
#endif
} cpymo_package_stream_reader;

cpymo_package_stream_reader cpymo_package_stream_reader_create(
	const cpymo_package *package, 
	const cpymo_package_index *index);

error_t cpymo_package_stream_reader_find_create(
	cpymo_package_stream_reader *r,
	const cpymo_package *package,
	cpymo_str name);

error_t cpymo_package_stream_reader_seek(
	size_t seek,
	cpymo_package_stream_reader *r);

size_t cpymo_package_stream_reader_read(
	char *dst_buf,
	size_t dst_buf_size,
	cpymo_package_stream_reader *r);

static inline bool cpymo_package_stream_reader_eof(cpymo_package_stream_reader *r)
{ return r->current >= r->file_length; }

static inline void cpymo_package_stream_reader_seek_cur(
	intptr_t seek,
	cpymo_package_stream_reader *r)
{ 
	cpymo_package_stream_reader_seek(r->current + seek, r);
}

void cpymo_package_stream_reader_close(cpymo_package_stream_reader *r);

#endif
