#include <stdio.h>
#include <string.h>
#include <cpymo_error.h>
#include "package.h"

void help() {
	printf("cpymo-tool\n");
	printf("Development tool for cpymo.\n");
	printf("\n");
	printf("Unpack a pymo package:\n");
	printf("    cpymo-tool unpack <pak-file> <extension_with \".\"> <output-dir>\n");
}

int process_err(error_t e) {
	if (e == CPYMO_ERR_SUCC) return 0;
	else {
		printf("Error: %s", cpymo_error_message(e));
		return -1;
	}
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
				return process_err(unpack(pak, ext, out));
			}
			else help();
		}
		else help();
	}

	return 0;
}
