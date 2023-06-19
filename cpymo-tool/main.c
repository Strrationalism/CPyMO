#include "cpymo_tool_prelude.h"

// import modules from CPyMO
#include "../cpymo/cpymo_error.c"
#include "../cpymo/cpymo_package.c"
#include "../cpymo/cpymo_parser.c"
#include "../cpymo/cpymo_utils.c"
#include "../cpymo/cpymo_color.c"
#include "../cpymo/cpymo_gameconfig.c"
#include "../cpymo/cpymo_assetloader.c"
#include "../cpymo/cpymo_album.c"
#include "../cpymo/cpymo_str.c"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../stb/stb_image_resize.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

#define STB_DS_IMPLEMENTATION
#include "../stb/stb_ds.h"

#ifdef LEAKCHECK
#define STB_LEAKCHECK_IMPLEMENTATION
#include "../stb/stb_leakcheck.h"
#endif

extern int cpymo_tool_invoke_generate_album_ui(int argc, const char **argv);
extern int cpymo_tool_invoke_pack(int argc, const char **argv);
extern int cpymo_tool_invoke_unpack(int argc, const char **argv);
extern int cpymo_tool_invoke_resize_image(int argc, const char **argv);
extern int cpymo_tool_invoke_pack_spritesheet(int argc, const char **argv);

int help(void) {
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
		"    cpymo-tool resize-image \n"
		"        <src-image-file> <dst-image-file> <resize-ratio-w> <resize-ratio-h>\n"
		"        [--load-mask [png/bmp/jpg]] [--create-mask [png/bmp/jpg]] [--out-format <png/bmp/jpg>]\n");
	printf("Pack images into spritesheet:\n");
	printf(
		"    cpymo-tool pack-spritesheet\n"
		"        <output-file> <num-of-cols> <input-files...>\n"
		"        [--load-mask] [--create-mask] [--out-format <png/bmp/jpg>]\n");
	printf("Generate album UI image cache:\n");
	printf(
		"    cpymo-tool gen-album-cache <gamedir> [additional-album-lists...]\n");
	printf("Strip pymo game:\n");
	printf("    cpymo-tool strip <gamedir> <output-gamedir>\n");
	printf("Convert pymo game:\n");
	printf("    cpymo-tool conv <s60v3/s60v5/pymo/3ds/psp/wii> <gamedir> <output-gamedir>\n");
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
	int ret = 0;
	if (argc < 2) {
		ret = help();
	}
	else {
		extern int cpymo_tool_invoke_strip(int argc, const char **argv);

		if (strcmp(argv[1], "unpack") == 0)
			ret = cpymo_tool_invoke_unpack(argc, argv);
		else if (strcmp(argv[1], "pack") == 0)
			ret = cpymo_tool_invoke_pack(argc, argv);
		else if (strcmp(argv[1], "resize-image") == 0)
			ret = cpymo_tool_invoke_resize_image(argc, argv);
		else if (strcmp(argv[1], "pack-spritesheet") == 0)
			ret = cpymo_tool_invoke_pack_spritesheet(argc, argv);
		else if (strcmp(argv[1], "gen-album-cache") == 0)
			ret = cpymo_tool_invoke_generate_album_ui(argc, argv);
		else if (strcmp(argv[1], "strip") == 0)
			ret = cpymo_tool_invoke_strip(argc, argv);
		else ret = help();
	}

	#ifdef LEAKCHECK
	stb_leakcheck_dumpmem();
	#endif

	return ret;
}

