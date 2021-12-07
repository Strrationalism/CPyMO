#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cpymo_package.h"

void help() {
	printf("cpymo-tool\n");
	printf("Development tool for cpymo.\n");
	printf("\n");
	printf("Unpack a pymo package:\n");
	printf("    cpymo-tool unpack <pak-file> <extension_without \".\"> <output-dir>\n");
}

int unpack(const char *pak_path, const char *extension, const char *out_path) {
	cpymo_package pkg;

	if (cpymo_package_open(&pkg, pak_path) != CPYMO_ERR_SUCC) {
		printf("Error: Can not open package file.\n");
		return -1;
	}

	for (uint32_t i = 0; i < pkg.file_count; ++i) {
		const cpymo_package_index *file_index = &pkg.files[i];
		printf("%s.%s\n", file_index->file_name, extension);

		char out_file_path[256] = { '\0' };
		strcat(out_file_path, out_path);
		strcat(out_file_path, "/");
		strcat(out_file_path, file_index->file_name);
		strcat(out_file_path, ".");
		strcat(out_file_path, extension);

		FILE *out = fopen(out_file_path, "wb");
		if (out == NULL) {
			printf("Error: Can not write %s.\n", out_file_path);
			continue;
		}

		char *buf = malloc(file_index->file_length);
		if (buf == NULL) {
			printf("Error: Out of memory.");
			continue;
		}

		error_t err = cpymo_package_read_file(buf, &pkg, file_index);
		if (err != CPYMO_ERR_SUCC) {
			printf("Error: Can not read file, error code: %d.\n", err);
			free(buf);
			fclose(out);
			continue;
		}

		if (fwrite(buf, file_index->file_length, 1, out) != 1) {
			printf("Error: Can not write to file.");
		}

		free(buf);
		fclose(out);
	}

	return 0;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		help();
	}
	else {
		if (strcmp(argv[1], "unpack") == 0) {
			if (argc == 5) {
				const char *pak = argv[2];
				const char *ext = argv[3];
				const char *out = argv[4];
				return unpack(pak, ext, out);
			}
			else help();
		}
		else help();
	}

	return 0;
}