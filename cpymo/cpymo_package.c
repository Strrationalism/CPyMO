#include "cpymo_package.h"
#include "cpymo_utils.h"

#include <string.h>
#include <stdlib.h>
#include <endianness.h>

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

	for (uint32_t i = 0; i < out_package->file_count; ++i) {
		cpymo_package_index *file = &out_package->files[i];
		file->file_length = end_le32toh(file->file_length);
		file->file_offset = end_le32toh(file->file_offset);
	}

	if (count != out_package->file_count) {
		cpymo_package_close(out_package);
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	return CPYMO_ERR_SUCC;
}

void cpymo_package_close(cpymo_package * package)
{
	free(package->files);
	fclose(package->stream);
}

error_t cpymo_package_find(cpymo_package_index *out_index, const cpymo_package *package, const char * filename)
{
	for (uint32_t i = 0; i < package->file_count; ++i) {
		if (cpymo_utils_string_equals_ignore_case(package->files[i].file_name, filename)) {
			*out_index = package->files[i];
			return CPYMO_ERR_SUCC;
		}
	}

	return CPYMO_ERR_NOT_FOUND;
}

error_t cpymo_package_read_file(char *out_buffer, const cpymo_package * package, const cpymo_package_index * index)
{
	fseek(package->stream, index->file_offset, SEEK_SET);
	const size_t count = fread(out_buffer, index->file_length, 1, package->stream);

	if (count != 1) return CPYMO_ERR_BAD_FILE_FORMAT;

	return CPYMO_ERR_SUCC;
}


error_t cpymo_package_stream_reader_find_create(cpymo_package_stream_reader * r, const cpymo_package * package, cpymo_parser_stream_span filename)
{
	cpymo_package_index index;
	char name[32];
	cpymo_parser_stream_span_copy(name, sizeof(name), filename);
	
	error_t err = cpymo_package_find(&index, package, name);
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
