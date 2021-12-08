#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <cpymo_package.h>

error_t unpack(const char *pak_path, const char *extension, const char *out_path) {
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

		printf("%s%s\n", filename, extension);

		char out_file_path[256] = { '\0' };
		strcat(out_file_path, out_path);
		strcat(out_file_path, "/");
		strcat(out_file_path, filename);
		strcat(out_file_path, extension);

		FILE *out = fopen(out_file_path, "wb");
		if (out == NULL) {
			printf("Error: Can not write %s.\n", out_file_path);
			continue;
		}

		error_t err = cpymo_package_read_file(buf, &pkg, file_index);
		if (err != CPYMO_ERR_SUCC) {
			printf("Error: Can not read file, error code: %d.\n", err);
			fclose(out);
			continue;
		}

		if (fwrite(buf, file_index->file_length, 1, out) != 1) {
			printf("Error: Can not write to file.");
		}

		fclose(out);
	}

	free(buf);

	return CPYMO_ERR_SUCC;
}
