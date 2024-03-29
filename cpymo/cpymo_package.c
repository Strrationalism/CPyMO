﻿#include "cpymo_prelude.h"
#include "cpymo_package.h"
#include "cpymo_utils.h"

#include <string.h>
#include <stdlib.h>
#include "../endianness.h/endianness.h"
#include "../stb/stb_image.h"
#include <assert.h>

error_t cpymo_package_open(cpymo_package *out_package, const char * path)
{
#ifdef DEBUG
	out_package->has_stream_reader = false;
#endif
	
	if (out_package == NULL) return CPYMO_ERR_INVALID_ARG;

	out_package->stream = fopen(path, "rb");
	if (out_package->stream == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	size_t count = 
		fread(
			&out_package->file_count, 
			sizeof(out_package->file_count), 
			1, 
			out_package->stream);

	out_package->file_count = end_le32toh(out_package->file_count);

	if (count != 1) {
		fclose(out_package->stream);
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	out_package->files = (cpymo_package_index *)malloc(sizeof(cpymo_package_index) * out_package->file_count);

	if (out_package->files == NULL) return CPYMO_ERR_OUT_OF_MEM;
	
	count =
		fread(
			out_package->files,
			sizeof(cpymo_package_index),
			out_package->file_count,
			out_package->stream);

	if (count != out_package->file_count) {
		cpymo_package_close(out_package);
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	for (uint32_t i = 0; i < out_package->file_count; ++i) {
		cpymo_package_index *file = &out_package->files[i];
		file->file_length = end_le32toh(file->file_length);
		file->file_offset = end_le32toh(file->file_offset);
	}

	return CPYMO_ERR_SUCC;
}

void cpymo_package_close(cpymo_package * package)
{
	free(package->files);
	fclose(package->stream);
}

error_t cpymo_package_find(cpymo_package_index * out_index, const cpymo_package * package, cpymo_str filename)
{
	if (filename.len > 31) {
		filename.len = 31;
		printf("[Warning] File name \"");
		for (size_t i = 0; i < filename.len; ++i)
			putchar(filename.begin[i]);
		puts("\" is too long!");
	}

	for (uint32_t i = 0; i < package->file_count; ++i) {
		if (cpymo_str_equals_str_ignore_case(filename, package->files[i].file_name)) {
			*out_index = package->files[i];
			return CPYMO_ERR_SUCC;
		}
	}

	return CPYMO_ERR_NOT_FOUND;
}

error_t cpymo_package_read_file_from_index(char *out_buffer, const cpymo_package * package, const cpymo_package_index * index)
{
	#ifdef DEBUG
	assert(package->has_stream_reader == false);
	#endif
	
	fseek(package->stream, index->file_offset, SEEK_SET);
	const size_t count = fread(out_buffer, index->file_length, 1, package->stream);

	if (count != 1) return CPYMO_ERR_BAD_FILE_FORMAT;

	return CPYMO_ERR_SUCC;
}

error_t cpymo_package_read_file(char **out_buffer, size_t *sz, const cpymo_package *package, cpymo_str filename)
{
	assert(*out_buffer == NULL);
	cpymo_package_index idx;
	error_t err = cpymo_package_find(&idx, package, filename);
	CPYMO_THROW(err);

	*sz = idx.file_length;
	*out_buffer = (char *)malloc(*sz);
	if (*out_buffer == NULL) return CPYMO_ERR_OUT_OF_MEM;

	return cpymo_package_read_file_from_index(*out_buffer, package, &idx);
}

#ifdef STREAMING_LOAD_IMAGE

static int cpymo_package_stream_read_image_eof(void *stream_reader)
{
	return cpymo_package_stream_reader_eof((cpymo_package_stream_reader *)stream_reader);
}

static int cpymo_package_stream_reader_image_read(void *stream_reader, char *data, int size)
{
	return (int)cpymo_package_stream_reader_read(data, (size_t)size, (cpymo_package_stream_reader *)stream_reader);
}

static void cpymo_package_stream_reader_image_skip(void *stream_reader, int n)
{
	cpymo_package_stream_reader_seek_cur(n, (cpymo_package_stream_reader *)stream_reader);
}

#endif

#ifndef DISABLE_STB_IMAGE
error_t cpymo_package_read_image_from_index(void ** pixels, int * w, int * h, int channels, const cpymo_package * pkg, const cpymo_package_index * index)
{	
#ifdef STREAMING_LOAD_IMAGE
	stbi_io_callbacks cbs;
	cbs.eof = &cpymo_package_stream_read_image_eof;
	cbs.read = &cpymo_package_stream_reader_image_read;
	cbs.skip = &cpymo_package_stream_reader_image_skip;

	cpymo_package_stream_reader sr = cpymo_package_stream_reader_create(pkg, index);

	*pixels = stbi_load_from_callbacks(&cbs, &sr, w, h, NULL, channels);

	cpymo_package_stream_reader_close(&sr);

	if (*pixels == NULL) {
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	return CPYMO_ERR_SUCC;

#else
	stbi_uc *file_data = (stbi_uc *)malloc(index->file_length);
	if (file_data == NULL) return CPYMO_ERR_OUT_OF_MEM;

	error_t err = cpymo_package_read_file_from_index((char *)file_data, pkg, index);
	if (err != CPYMO_ERR_SUCC) {
		free(file_data);
		return err;
	}

	// Why this method is faster than stream loading in package ???
	*pixels = stbi_load_from_memory(file_data, index->file_length, w, h, NULL, channels);
	free(file_data);
	
	if (*pixels == NULL) {
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	return CPYMO_ERR_SUCC;
#endif
}

error_t cpymo_package_read_image(void ** pixels, int * w, int * h, int channels, const cpymo_package * pkg, cpymo_str filename)
{
	cpymo_package_index i;
	error_t err = cpymo_package_find(&i, pkg, filename);
	CPYMO_THROW(err);
	
	return cpymo_package_read_image_from_index(pixels, w, h, channels, pkg, &i);
}
#endif

error_t cpymo_package_stream_reader_find_create(cpymo_package_stream_reader * r, const cpymo_package * package, cpymo_str filename)
{	
	cpymo_package_index index;
	error_t err = cpymo_package_find(&index, package, filename);
	CPYMO_THROW(err);

	*r = cpymo_package_stream_reader_create(package, &index);

	return CPYMO_ERR_SUCC;
}

error_t cpymo_package_stream_reader_seek(size_t seek, cpymo_package_stream_reader *r)
{
	if (seek > r->file_length) {
		return CPYMO_ERR_OUT_OF_MEM;
	}

	r->current = seek;
	fseek(r->stream, (long)(r->file_offset + r->current), SEEK_SET);
	
	return CPYMO_ERR_SUCC;
}

size_t cpymo_package_stream_reader_read(char *dst_buf, size_t dst_buf_size, cpymo_package_stream_reader * r)
{
	size_t read_size = r->file_length - r->current;
	if (read_size > dst_buf_size) read_size = dst_buf_size;

	if (read_size <= 0) return 0;

	r->current += read_size;

	return fread(dst_buf, read_size, 1, r->stream) * read_size;
}

void cpymo_package_stream_reader_close(cpymo_package_stream_reader * r)
{
	if (r->own_stream && r->stream) fclose(r->stream);
#ifdef DEBUG
	if (r->package) r->package->has_stream_reader = false;
#endif

#ifdef LEAKCHECK
	free(r->leak_mark);
#endif
}

cpymo_package_stream_reader cpymo_package_stream_reader_create(
	const cpymo_package * package,
	const cpymo_package_index * index)
{
	cpymo_package_stream_reader reader;
	reader.file_offset = index->file_offset;
	reader.file_length = index->file_length;
	reader.current = 0;
	reader.stream = package->stream;
	reader.own_stream = false;
#ifdef DEBUG
	assert(package->has_stream_reader == false);
	reader.package = (cpymo_package *)package;
#endif

#ifdef LEAKCHECK
	reader.leak_mark = malloc(1024);
	assert(reader.leak_mark);
#endif
	cpymo_package_stream_reader_seek(0, &reader);
	return reader;
}

error_t cpymo_package_stream_reader_from_file(
	cpymo_package_stream_reader *out,
	const char *path)
{
	FILE *file = fopen(path, "rb");
	if (file == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

#ifdef LEAKCHECK
	void *leak_mark = malloc(1024);
	if (leak_mark == NULL) {
		fclose(file);
		return CPYMO_ERR_OUT_OF_MEM;
	}

	out->leak_mark = leak_mark;
#endif

#ifdef DEBUG
	out->package = NULL;
#endif

	fseek(file, 0, SEEK_END);

	out->current = 0;
	out->file_length = (size_t)ftell(file);
	out->file_offset = 0;
	out->own_stream = true;
	out->stream = file;

	fseek(file, 0, SEEK_SET);

	return CPYMO_ERR_SUCC;
}
