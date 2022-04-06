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

void help() {
	printf("cpymo-tool\n");
	printf("Development tool for pymo and cpymo.\n");
	printf("\n");
	printf("Unpack a pymo package:\n");
	printf("    cpymo-tool unpack <pak-file> <extension-with \".\"> <output-dir>\n");
	printf("Pack a pymo package:\n");
	printf("    cpymo-tool pack <out-pak-file> <files-to-pack...>\n");
	printf("    cpymo-tool pack <out-pak-file> --file-list <file-list.txt>\n");
	printf("Resize image:\n");
	printf(
		"    cpymo-tool resize \n"
		"        <src-image-file> <dst-image-file> <resize-ratio-w> <resize-ratio-h>\n"
		"        [--load-mask] [--create-mask] [--out-format <png/bmp/jpg>]");
	printf("\n");
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
			else help();
		}
		else if (strcmp(argv[1], "resize") == 0) {
			const char *src_file = NULL;
			const char *dst_file = NULL;
			const char *resize_ratio_w = NULL;
			const char *resize_ratio_h = NULL;
			bool load_mask = false, create_mask = false;
			const char *out_format = NULL;

			for (int i = 2; i < argc; ++i) {
				cpymo_parser_stream_span a = cpymo_parser_stream_span_pure(argv[i]);
				cpymo_parser_stream_span_trim(&a);
				
				if (cpymo_parser_stream_span_equals_str(a, "")) continue;
				else if (cpymo_parser_stream_span_starts_with_str_ignore_case(a, "--")) {
					if (cpymo_parser_stream_span_equals_str(a, "--load-mask"))
						load_mask = true;
					else if (cpymo_parser_stream_span_equals_str(a, "--create-mask"))
						create_mask = true;
					else if (cpymo_parser_stream_span_equals_str(a, "--out-format")) {
						++i;
						
						if (argc <= i) {
							printf("[Error] --out-format requires an argument.\n");
							help();
							return -1;
						}
						
						out_format = argv[i];
					}
					else {
						printf("[Error] Unknown option: %s\n", argv[i]);
						help();
						return -1;
					}
				}
				else if (src_file == NULL) src_file = argv[i];
				else if (dst_file == NULL) dst_file = argv[i];
				else if (resize_ratio_w == NULL) resize_ratio_w = argv[i];
				else if (resize_ratio_h == NULL) resize_ratio_h = argv[i];
				else {
					printf("[Error] Unknown argument: %s\n", argv[i]);
					help();
					return -1;
				}
			}

			if (src_file == NULL) {
				printf("[Error] No source file specified.\n");
				help();
				return -1;
			}
			else if (dst_file == NULL) {
				printf("[Error] No destination file specified.\n");
				help();
				return -1;
			}
			else if (resize_ratio_w == NULL || resize_ratio_h == NULL) {
				printf("[Error] No resize ratio specified.\n");
				help();
				return -1;
			}

			double ratio_w = atof(resize_ratio_w);
			double ratio_h = atof(resize_ratio_h);

			if (out_format == NULL) {
				out_format = strrchr(dst_file, '.');
				if (out_format) out_format++;
			}

			if (out_format == NULL) {
				printf("[Error] Can not get image format, please specify it with --out-format.\n");
				return -1;
			}
				
			error_t err = 
				cpymo_tool_resize_image(src_file, dst_file, ratio_w, ratio_h, load_mask, create_mask, out_format);
			return process_err(err);
			
		}
		else help();
	}

	return 0;
}
