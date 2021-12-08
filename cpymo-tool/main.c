#include <stdio.h>
#include <string.h>
#include <cpymo_error.h>
#include "cpymo_tool_package.h"

void help() {
	printf("cpymo-tool\n");
	printf("Development tool for pymo and cpymo.\n");
	printf("\n");
	printf("Unpack a pymo package:\n");
	printf("    cpymo-tool unpack <pak-file> <extension-with \".\"> <output-dir>\n");
	printf("Pack a pymo package:\n");
	printf("    cpymo-tool pack <out-pak-file> <files-to-pack...>");
	printf("\n");
}

int process_err(error_t e) {
	if (e == CPYMO_ERR_SUCC) return 0;
	else {
		printf("Error: %s", cpymo_error_message(e));
		return -1;
	}
}

int main(int argc, const char **argv) {
	if (argc < 2) {
		help();
	}
	else {
		if (strcmp(argv[1], "unpack") == 0) {
			if (argc == 5) {
				const char *pak = argv[2];
				const char *ext = argv[3];
				const char *out = argv[4];
				return process_err(cpymo_tool_unpack(pak, ext, out));
			}
			else help();
		}
		else if (strcmp(argv[1], "pack") == 0) {
			if (argc >= 4) {
				const char *out_pak = argv[2];
				const char **files_to_pack = argv + 3;
				return process_err(cpymo_tool_pack(out_pak, files_to_pack, (uint32_t)argc - 3));
			}
			else help();
		}
		else help();
	}

	return 0;
}
