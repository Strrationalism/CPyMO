#include "cpymo_tool_resize.h"
#include <stdbool.h>
#include <cpymo_error.h>
#include <stb_image.h>
#include <stb_image_resize.h>
#include <stb_image_write.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <cpymo_utils.h>
#include <stdint.h>
#include <cpymo_parser.h>
#include "cpymo_tool_image.h"

static error_t cpymo_tool_resize_image(
    const char *input_file, 
    const char *output_file, 
    double ratio_w, 
    double ratio_h, 
    bool load_mask, 
    bool create_mask, 
    const char * out_format)
{
    cpymo_tool_image input;
    error_t err = cpymo_tool_image_load_from_file(&input, input_file, load_mask);

    {
        cpymo_tool_image resized;
        err = cpymo_tool_image_resize(&resized, &input, (size_t)(ratio_w * input.width), (size_t)(ratio_h * input.height));
        cpymo_tool_image_free(input);
        input = resized;

        if (err != CPYMO_ERR_SUCC) {
            cpymo_tool_image_free(input);
            return err;
        }
    }

    if (create_mask) {
        {
            cpymo_tool_image mask;
            err = cpymo_tool_image_create_mask(&mask, &input);
            if (err != CPYMO_ERR_SUCC) {
                printf("[Warning] Can not create mask: %s(%s).\n", output_file, cpymo_error_message(err));
                goto MASK_FAILED;
            }

            char *mask_name = NULL;
            err = cpymo_tool_get_mask_name(&mask_name, output_file);
            if (err != CPYMO_ERR_SUCC) {
                cpymo_tool_image_free(mask);
                printf("[Warning] Can not get mask name: %s(%s).\n", output_file, cpymo_error_message(err));
                goto MASK_FAILED;
            }

            err = cpymo_tool_image_save_to_file(&mask, mask_name, cpymo_parser_stream_span_pure(out_format));
            free(mask_name);
            cpymo_tool_image_free(mask);

            if (err != CPYMO_ERR_SUCC) {
                printf("[Warning] Failed to save mask image: %s(%s).\n", output_file, cpymo_error_message(err));
                goto MASK_FAILED;
            }
        }

        cpymo_tool_image rgb;
        err = cpymo_tool_image_copy_without_mask(&rgb, &input);
        if (err == CPYMO_ERR_SUCC) {
            cpymo_tool_image_free(input);
            input = rgb;
        }
    }

MASK_FAILED:
    err = cpymo_tool_image_save_to_file(&input, output_file, cpymo_parser_stream_span_pure(out_format));
    cpymo_tool_image_free(input);

    return err;
}

extern int help();
extern int process_err(error_t);

int cpymo_tool_invoke_resize(int argc, const char ** argv)
{
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

	error_t err =
		cpymo_tool_resize_image(src_file, dst_file, ratio_w, ratio_h, load_mask, create_mask, out_format);
	return process_err(err);
}
