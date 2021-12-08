#include "cpymo_package.h"

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

	out_package->files = malloc(sizeof(cpymo_package_index) * out_package->file_count);

	if (out_package->files == NULL) return CPYMO_ERR_OUT_OF_MEM;
	
	#pragma warning(disable: 6029)
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
		if (strcmp(package->files[i].file_name, filename) == 0) {
			*out_index = package->files[i];
			return CPYMO_ERR_SUCC;
		}
	}

	return CPYMO_ERR_NOT_FOUND;
}

error_t cpymo_package_read_file(char * out_buffer, const cpymo_package * package, const cpymo_package_index * index)
{
	fseek(package->stream, index->file_offset, SEEK_SET);
	const size_t count = fread(out_buffer, index->file_length, 1, package->stream);

	if (count != 1) return CPYMO_ERR_BAD_FILE_FORMAT;

	return CPYMO_ERR_SUCC;
}
