#include "cpymo_tool_prelude.h"
#include "cpymo_tool_image.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "../cpymo/cpymo_error.h"
#include "../cpymo/cpymo_parser.h"
#include "../cpymo/cpymo_utils.h"
#include "../stb/stb_image.h"
#include "../stb/stb_image_resize.h"
#include "../stb/stb_image_write.h"


static error_t cpymo_tool_resize_image(
    const char *input_file, 
    const char *output_file, 
    double ratio_w, 
    double ratio_h, 
    bool load_mask, 
    bool create_mask, 
    const char * out_format)
{
    cpymo_tool_image img;
    error_t err = cpymo_tool_image_load_from_file(&img, input_file, load_mask);
	CPYMO_THROW(err);

    
	cpymo_tool_image resized;
	err = cpymo_tool_image_resize(&resized, &img, (size_t)(ratio_w * img.width), (size_t)(ratio_h * img.height));
	cpymo_tool_image_free(img);
	CPYMO_THROW(err);

    err = cpymo_tool_image_save_to_file_with_mask(
		&resized, output_file, cpymo_str_pure(out_format), create_mask);
	cpymo_tool_image_free(resized);
	return err;
}

extern int help();
extern int process_err(error_t);

int cpymo_tool_invoke_resize_image(int argc, const char ** argv)
{
	const char *src_file = NULL;
	const char *dst_file = NULL;
	const char *resize_ratio_w = NULL;
	const char *resize_ratio_h = NULL;
	bool load_mask = false, create_mask = false;
	const char *out_format = NULL;

	for (int i = 2; i < argc; ++i) {
		cpymo_str a = cpymo_str_pure(argv[i]);
		cpymo_str_trim(&a);

		if (cpymo_str_equals_str(a, "")) continue;
		else if (cpymo_str_starts_with_str_ignore_case(a, "--")) {
			if (cpymo_str_equals_str(a, "--load-mask"))
				load_mask = true;
			else if (cpymo_str_equals_str(a, "--create-mask"))
				create_mask = true;
			else if (cpymo_str_equals_str(a, "--out-format")) {
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

	if (dst_file == NULL) {
		printf("[Error] No destination file specified.\n");
		help();
		return -1;
	}

	if (resize_ratio_w == NULL || resize_ratio_h == NULL) {
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

	if (strcmp(resize_ratio_w, "1") == 0 && strcmp(resize_ratio_h, "1") == 0) {
		cpymo_tool_image img;
		error_t err = cpymo_tool_image_load_from_file(&img, src_file, load_mask);
		if (err != CPYMO_ERR_SUCC) return process_err(err);

		err = cpymo_tool_image_save_to_file_with_mask(&img, dst_file, cpymo_str_pure(out_format), create_mask);
		cpymo_tool_image_free(img);
		return process_err(err);
	}

	error_t err =
		cpymo_tool_resize_image(src_file, dst_file, ratio_w, ratio_h, load_mask, create_mask, out_format);
	return process_err(err);
}

