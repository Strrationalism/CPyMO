#include "cpymo_tool_prelude.h"
#include "cpymo_tool_package.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "../cpymo/cpymo_package.h"
#include "../cpymo/cpymo_utils.h"
#include "../endianness.h/endianness.h"

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
	cpymo_package_close(&pkg);

	return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_package_packer_open(
    cpymo_tool_package_packer *packer,
    const char *path,
    size_t max_files_count)
{
	packer->current_file_count = 0;
	packer->data_section_start_offset =
		sizeof(uint32_t) + max_files_count * sizeof(cpymo_package_index);
	packer->index_section_start_offset = sizeof(uint32_t);
	packer->max_file_count = max_files_count;

	packer->stream = fopen(path, "wb");
	if (packer->stream == NULL)
		return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	for (long i = 0; i < packer->data_section_start_offset; ++i)
		fputc(0, packer->stream);

	return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_package_packer_add_data(
    cpymo_tool_package_packer *packer,
    cpymo_str name,
    void *data,
    size_t len)
{
	if (packer->current_file_count >= packer->max_file_count)
		return CPYMO_ERR_NO_MORE_CONTENT;

	fseek(packer->stream, packer->index_section_start_offset, SEEK_SET);
	char filename[32] = { '\0' };
	cpymo_str_copy(filename, sizeof(filename), name);
	size_t written = fwrite(filename, sizeof(filename), 1, packer->stream);
	if (written != 1) return CPYMO_ERR_UNKNOWN;

	uint32_t index[2] = {
		end_htole32(packer->data_section_start_offset),
		end_htole32((uint32_t)len)
	};

	written = fwrite(index, sizeof(index), 1, packer->stream);
	if (written != 1) return CPYMO_ERR_UNKNOWN;
	long new_index_offset = ftell(packer->stream);

	fseek(packer->stream, packer->data_section_start_offset, SEEK_SET);
	written = fwrite(data, len, 1, packer->stream);
	if (written != 1) return CPYMO_ERR_UNKNOWN;
	long new_data_offset = ftell(packer->stream);

	packer->current_file_count++;
	packer->data_section_start_offset = new_data_offset;
	packer->index_section_start_offset = new_index_offset;
	return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_package_packer_add_file(
    cpymo_tool_package_packer *packer,
    const char *file)
{
	const char *filename_start1 = strrchr(file, '/') + 1;
	const char *filename_start2 = strrchr(file, '\\') + 1;
	const char *filename = filename_start1;
	if (filename_start2 > filename) filename = filename_start2;
	if (file > filename) filename = file;
	const char *ext_start = strrchr(filename, '.');

	char filename_index[32] = { '\0' };

	size_t j = 0;
	bool finished = false;
	for (size_t j = 0; j < 31; ++j) {
		if (filename + j == ext_start || filename[j] == '\0') {
			finished = true;
			break;
		}
		else
			filename_index[j] = toupper(filename[j]);
	}

	if (!finished)
		printf("[Warning] File name \"%s\" is too long!\n", filename_index);

	char *data = NULL;
	size_t len;
	error_t err = cpymo_utils_loadfile(file, &data, &len);
	CPYMO_THROW(err);

	err = cpymo_tool_package_packer_add_data(
		packer, cpymo_str_pure(filename_index), data, len);
	free(data);
	return err;
}

void cpymo_tool_package_packer_close(
    cpymo_tool_package_packer *packer)
{
	uint32_t filecount_le32 = end_htole32((uint32_t)packer->current_file_count);
	fseek(packer->stream, 0, SEEK_SET);
	fwrite(&filecount_le32, sizeof(filecount_le32), 1, packer->stream);
	fclose(packer->stream);
}

static error_t cpymo_tool_pack(const char *out_pack_path, const char **files_to_pack, uint32_t file_count)
{
	cpymo_tool_package_packer p;
	error_t err = cpymo_tool_package_packer_open(&p, out_pack_path, file_count);
	CPYMO_THROW(err);

	for (uint32_t i = 0; i < file_count; ++i) {
		printf("%s\n", files_to_pack[i]);
		err = cpymo_tool_package_packer_add_file(&p, files_to_pack[i]);
		if (err != CPYMO_ERR_SUCC) {
			printf("[Error] Can not pack file: %s.\n", files_to_pack[i]);
			cpymo_tool_package_packer_close(&p);
			return err;
		}
	}

	cpymo_tool_package_packer_close(&p);
	printf("==> %s\n", out_pack_path);

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
		cpymo_str line = cpymo_parser_curline_readuntil(&parser, '\n');
		cpymo_str_trim(&line);

		if (line.len) {
			char **cur = *files + *count;
			*cur = (char *)malloc(line.len + 1);
			if (*cur == NULL) return CPYMO_ERR_OUT_OF_MEM;

			cpymo_str_copy(*cur, line.len + 1, line);
			++*count;
		}
	} while (cpymo_parser_next_line(&parser));

	free(ls_buf);

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


