#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <cpymo_error.h>
#include <cpymo_parser.h>
#include "cpymo_tool_package.h"
#include "cpymo_tool_resize.h"

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

int help() {
	printf("cpymo-tool\n");
	printf("Development tool for PyMO and CPyMO.\n");
	printf("\n");
	printf("Unpack a PyMO package:\n");
	printf("    cpymo-tool unpack <pak-file> <extension-with \".\"> <output-dir>\n");
	printf("Pack a PyMO package:\n");
	printf("    cpymo-tool pack <out-pak-file> <files-to-pack...>\n");
	printf("    cpymo-tool pack <out-pak-file> --file-list <file-list.txt>\n");
	printf("Resize image:\n");
	printf(
		"    cpymo-tool resize \n"
		"        <src-image-file> <dst-image-file> <resize-ratio-w> <resize-ratio-h>\n"
		"        [--load-mask] [--create-mask] [--out-format <png/bmp/jpg>]\n");
	printf("Pack images to single image:\n");
	printf(
		"    cpymo-tool pack-images\n"
		"        <output-file> <num-of-cols> <input-files...>\n"
		"        [--load-mask] [--create-mask] [--out-format <png/bmp/jpg>]\n");
	printf("\n");
	return 0;
}

int process_err(error_t e) {
	if (e == CPYMO_ERR_SUCC) return 0;
	else {
		printf("[Error] %s\n", cpymo_error_message(e));
		return -1;
	}
}

int main(int argc, const char **argv) {
	if (argc < 2) {
		return help();
	}
	else {
		if (strcmp(argv[1], "unpack") == 0) {
			return cpymo_tool_invoke_unpack(argc, argv);
		}
		else if (strcmp(argv[1], "pack") == 0) {
			return cpymo_tool_invoke_pack(argc, argv);
		}
		else if (strcmp(argv[1], "resize") == 0) {
			return cpymo_tool_resize_invoke(argc, argv);
		}
		else return help();
	}
}
