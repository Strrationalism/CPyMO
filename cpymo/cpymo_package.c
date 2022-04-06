#include "cpymo_package.h"
#include "cpymo_utils.h"

#include <string.h>
#include <stdlib.h>
#include <endianness.h>
#include <stb_image.h>

error_t cpymo_package_open(cpymo_package *out_package, const char * path)
{
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

error_t cpymo_package_find(cpymo_package_index * out_index, const cpymo_package * package, cpymo_parser_stream_span filename)
{
	for (uint32_t i = 0; i < package->file_count; ++i) {
		if (cpymo_parser_stream_span_equals_str_ignore_case(filename, package->files[i].file_name)) {
			*out_index = package->files[i];
			return CPYMO_ERR_SUCC;
		}
	}

	return CPYMO_ERR_NOT_FOUND;
}

error_t cpymo_package_read_file_from_index(char *out_buffer, const cpymo_package * package, const cpymo_package_index * index)
{
	fseek(package->stream, index->file_offset, SEEK_SET);
	const size_t count = fread(out_buffer, index->file_length, 1, package->stream);

	if (count != 1) return CPYMO_ERR_BAD_FILE_FORMAT;

	return CPYMO_ERR_SUCC;
}

static int cpymo_package_read_image_callbacks_read(void *u, char *d, int s)
{
	return cpymo_package_stream_reader_read(d, (size_t)s, (cpymo_package_stream_reader *)u);
}

static void cpymo_package_read_image_callbacks_skip(void *u, int n)
{
	cpymo_package_stream_reader_seek_cur(n, (cpymo_package_stream_reader *)u);
}

static int cpymo_package_reader_image_callback_eof(void *u)
{
	return cpymo_package_stream_reader_eof((cpymo_package_stream_reader *)u);
}

error_t cpymo_package_read_image_from_index(void ** pixels, int * w, int * h, int channels, const cpymo_package * pkg, const cpymo_package_index * index)
{
	const stbi_io_callbacks cb = {
		&cpymo_package_read_image_callbacks_read,
		&cpymo_package_read_image_callbacks_skip,
		&cpymo_package_reader_image_callback_eof
	};
	
	cpymo_package_stream_reader reader = cpymo_package_stream_reader_create(pkg, index);
	stbi_uc *px = stbi_load_from_callbacks(&cb, &reader, w, h, NULL, channels);

	if (px == NULL) {
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}
	
	*pixels = px;
	return CPYMO_ERR_SUCC;
}

error_t cpymo_package_read_image(void ** pixels, int * w, int * h, int channels, const cpymo_package * pkg, cpymo_parser_stream_span filename)
{
	cpymo_package_index i;
	error_t err = cpymo_package_find(&i, pkg, filename);
	CPYMO_THROW(err);
	
	return cpymo_package_read_image_from_index(pixels, w, h, channels, pkg, &i);
}

error_t cpymo_package_stream_reader_find_create(cpymo_package_stream_reader * r, const cpymo_package * package, cpymo_parser_stream_span filename)
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
	return CPYMO_ERR_SUCC;
}

size_t cpymo_package_stream_reader_read(char *dst_buf, size_t dst_buf_size, cpymo_package_stream_reader * r)
{
	size_t read_size = r->file_length - r->current;
	if (read_size > dst_buf_size) read_size = dst_buf_size;

	if (read_size <= 0) return 0;

	fseek(r->stream, (long)(r->file_offset + r->current), SEEK_SET);
	r->current += read_size;

	return fread(dst_buf, read_size, 1, r->stream) * read_size;
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
	return reader;
}
