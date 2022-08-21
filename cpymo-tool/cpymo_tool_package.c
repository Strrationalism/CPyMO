#include <cpymo_prelude.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <cpymo_package.h>
#include <endianness.h>
#include <cpymo_utils.h>
#include "cpymo_tool_package.h"

static error_t cpymo_tool_unpack(const char *pak_path, const char *extension, const char *out_path) {
	cpymo_package pkg;

	error_t err = cpymo_package_open(&pkg, pak_path);
	if (err != CPYMO_ERR_SUCC) return err;
	

	uint32_t max_length = 0;
	for (uint32_t i = 0; i < pkg.file_count; ++i) 
		if (pkg.files[i].file_length > max_length)
			max_length = pkg.files[i].file_length;
	
	char *buf = malloc(max_length);
	if (buf == NULL) {
		cpymo_package_close(&pkg);
		return CPYMO_ERR_OUT_OF_MEM;
	}

	for (uint32_t i = 0; i < pkg.file_count; ++i) {
		const cpymo_package_index *file_index = &pkg.files[i];

		char filename[32] = { '\0' };
		for (size_t i = 0; i < 32; ++i) {
			const char c = file_index->file_name[i];
			if (c == '\0') {
				filename[i] = '\0';
				break;
			}

			filename[i] = tolower(c);
		}

		char out_file_path[256] = { '\0' };
		strcat(out_file_path, out_path);
		strcat(out_file_path, "/");
		strcat(out_file_path, filename);
		strcat(out_file_path, extension);

		FILE *out = fopen(out_file_path, "wb");
		if (out == NULL) {
			printf("[Error] Can not write %s.\n", out_file_path);
			continue;
		}

		error_t err = cpymo_package_read_file_from_index(buf, &pkg, file_index);
		if (err != CPYMO_ERR_SUCC) {
			printf("[Error] Can not read file, error code: %d.\n", err);
			fclose(out);
			continue;
		}

		if (fwrite(buf, file_index->file_length, 1, out) != 1) {
			printf("[Error] Can not write to file.\n");
		}

		fclose(out);

		printf("%s%s\n", filename, extension);
	}

	free(buf);

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_tool_pack(const char *out_pack_path, const char **files_to_pack, uint32_t file_count)
{
	cpymo_package_index *index = malloc(sizeof(cpymo_package_index) * file_count);
	if (index == NULL) return CPYMO_ERR_OUT_OF_MEM;

	uint32_t current_offset = sizeof(uint32_t) + file_count * sizeof(cpymo_package_index);
	uint32_t max_length = 0;

	for (uint32_t i = 0; i < file_count; ++i) {
		const char *path = files_to_pack[i];
		FILE *file = fopen(path, "rb");
		if (file == NULL) {
			printf("Can not open file %s\n", path);
			free(index);
			return CPYMO_ERR_CAN_NOT_OPEN_FILE;
		}

		#pragma warning(disable: 6386)
		fseek(file, 0, SEEK_END);
		index[i].file_length = (uint32_t)ftell(file);
		fclose(file);

		index[i].file_offset = current_offset;

		#pragma warning(disable: 6385)
		current_offset += index[i].file_length;

		if (index[i].file_length > max_length)
			max_length = index[i].file_length;

		const char *filename_start1 = strrchr(path, '/') + 1;
		const char *filename_start2 = strrchr(path, '\\') + 1;
		const char *filename = filename_start1;

		if (filename_start2 > filename) filename = filename_start2;
		if (path > filename) filename = path;

		const char *ext_start = strrchr(filename, '.');

		size_t j = 0;
		bool finished = false;
		for (; j < 31; ++j) {
			if (filename + j == ext_start || filename[j] == '\0') {
				finished = true;
				break;
			}
			else
				index[i].file_name[j] = toupper(filename[j]);
		}

		for (; j < 32; ++j)
			index[i].file_name[j] = '\0';

		if (!finished) {
			printf("[Warning] File name \"%s\" is too long!\n", index[i].file_name);
		}

		index[i].file_length = end_htole32(index[i].file_length);
		index[i].file_offset = end_htole32(index[i].file_offset);
	}

	char *buf = malloc(max_length);
	if (buf == NULL) {
		free(index);
		return CPYMO_ERR_OUT_OF_MEM;
	}

	FILE *out_pak = fopen(out_pack_path, "wb");
	if (out_pak == NULL) {
		printf("[Error] Can not open %s.\n", out_pack_path);
		free(index);
		return CPYMO_ERR_CAN_NOT_OPEN_FILE;
	}

	uint32_t file_count_store = end_htole32(file_count);
	size_t count = fwrite(&file_count_store, sizeof(uint32_t), 1, out_pak);
	if (count != 1) {
		printf("[Error] Can not write file_count to package.\n");
		free(index);
		fclose(out_pak);
		return CPYMO_ERR_UNKNOWN;
	}

	count = fwrite(index, sizeof(cpymo_package_index), file_count, out_pak);
	free(index);

	if (count != file_count) {
		printf("[Error] Can not write file index to package.\n");
		fclose(out_pak);
	}

	for (uint32_t i = 0; i < file_count; ++i) {
		const char *path = files_to_pack[i];

		FILE *f = fopen(path, "rb");
		if (f == NULL) {
			printf("[Error] Can not open file %s.\n", path);
			fclose(out_pak);
			free(buf);
			return CPYMO_ERR_CAN_NOT_OPEN_FILE;
		}

		fseek(f, 0, SEEK_END);
		uint32_t length = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (fread(buf, length, 1, f) != 1) {
			printf("[Error] Can not read file %s.\n", path);
			fclose(out_pak);
			fclose(f);
			free(buf);
			return CPYMO_ERR_CAN_NOT_OPEN_FILE;
		}

		fclose(f);

		if (fwrite(buf, length, 1, out_pak) != 1) {
			printf("[Error] Can not write %s to package.\n", path);
			fclose(out_pak);
			free(buf);
			return CPYMO_ERR_UNKNOWN;
		}

		printf("%s\n", path);
	}

	free(buf);
	fclose(out_pak);

	printf("\n==> %s\n", out_pack_path);

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_tool_get_file_list(char *** files, size_t * count, const char * list_file)
{
	char *ls_buf = NULL;
	size_t ls_len;
	error_t err = cpymo_utils_loadfile(list_file, &ls_buf, &ls_len);
	CPYMO_THROW(err);
	
	cpymo_parser parser;
	cpymo_parser_init(&parser, ls_buf, ls_len);

	size_t reserve = 1;
	do {
		reserve++;
	} while (cpymo_parser_next_line(&parser));

	*files = (char **)malloc(reserve * sizeof(char *));
	if (*files == NULL) return CPYMO_ERR_OUT_OF_MEM;

	cpymo_parser_reset(&parser);

	*count = 0;
	do {
		cpymo_parser_stream_span line = cpymo_parser_curline_readuntil(&parser, '\n');
		cpymo_parser_stream_span_trim(&line);

		if (line.len) {
			char **cur = *files + *count;
			*cur = (char *)malloc(line.len + 1);
			if (*cur == NULL) return CPYMO_ERR_OUT_OF_MEM;

			cpymo_parser_stream_span_copy(*cur, line.len + 1, line);
			++*count;
		}
	} while (cpymo_parser_next_line(&parser));

	return CPYMO_ERR_SUCC;
}

extern int help();
extern int process_err(error_t);

int cpymo_tool_invoke_pack(int argc, const char ** argv)
{
	if (argc == 5) {
		if (strcmp(argv[3], "--file-list") == 0) {
			char **files = NULL;
			size_t filecount;
			error_t err = cpymo_tool_get_file_list(&files, &filecount, argv[4]);
			if (err == CPYMO_ERR_SUCC) {
				err = cpymo_tool_pack(argv[2], (const char **)files, (uint32_t)filecount);

				for (size_t i = 0; i < filecount; ++i)
					if (files[i]) free(files[i]);
				free(files);

				return process_err(err);
			}
			return process_err(err);
		}
		else goto NORMAL_PACK;
	}
	if (argc >= 4) {
	NORMAL_PACK: {
		const char *out_pak = argv[2];
		const char **files_to_pack = argv + 3;
		return process_err(cpymo_tool_pack(out_pak, files_to_pack, (uint32_t)argc - 3));
		}
	}
	else return help();
}

int cpymo_tool_invoke_unpack(int argc, const char ** argv)
{
	if (argc == 5) {
		const char *pak = argv[2];
		const char *ext = argv[3];
		const char *out = argv[4];
		return process_err(cpymo_tool_unpack(pak, ext, out));
	}
	else return help();
}


