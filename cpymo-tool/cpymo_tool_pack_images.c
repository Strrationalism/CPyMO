#include <cpymo_prelude.h>
#include "cpymo_tool_pack_images.h"
#include "cpymo_tool_image.h"
#include <cpymo_parser.h>
#include <cpymo_error.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern int help();
extern int process_err(error_t);

static error_t cpymo_tool_pack_images(
	bool load_mask,
	bool create_mask,
	const char *output_file,
	size_t cols,
	cpymo_parser_stream_span output_format,
	const char **input_files,
	size_t input_file_count) 
{
	cpymo_tool_image image;
	image.pixels = NULL;

	cpymo_tool_image *imgs = (cpymo_tool_image *)malloc(sizeof(cpymo_tool_image) * input_file_count);
	if (imgs == NULL) return CPYMO_ERR_OUT_OF_MEM;
	memset(imgs, 0, sizeof(cpymo_tool_image) * input_file_count);

	error_t err = CPYMO_ERR_SUCC;

	size_t max_width = 0, max_height = 0;
	for (size_t i = 0; i < input_file_count; ++i) {
		err = cpymo_tool_image_load_from_file(&imgs[i], input_files[i], load_mask);
		if (err != CPYMO_ERR_SUCC) {
			printf("[Error] Can not load image: %s(%s).\n", input_files[i], cpymo_error_message(err));
			goto CLEAN;
		}

		if (imgs[i].width > max_width) max_width = imgs[i].width;
		if (imgs[i].height > max_height) max_height = imgs[i].height;
	}

	size_t rows = input_file_count / cols;
	if (input_file_count % cols) rows++;
	
	err = cpymo_tool_image_create(&image, cols * max_width, rows * max_height, 4);
	if (err != CPYMO_ERR_SUCC) goto CLEAN;
	
	cpymo_tool_image_fill(&image, 0);

	for (size_t i = 0; i < input_file_count; ++i) {
		size_t x = (i % cols) * max_width;
		size_t y = (i / cols) * max_height;

		cpymo_tool_image_blit(&image, imgs + i, (int)x, (int)y);
	}

	err = cpymo_tool_image_save_to_file_with_mask(&image, output_file, output_format, create_mask);

CLEAN:
	for (size_t i = 0; i < input_file_count; ++i)
		if (imgs[i].pixels) cpymo_tool_image_free(imgs[i]);
	free(imgs);
	if (image.pixels) cpymo_tool_image_free(image);

	return err;
}

int cpymo_tool_invoke_pack_images(int argc, const char ** argv)
{
	bool load_mask = false;
	bool create_mask = false;
	const char *output_file = NULL;
	const char *num_of_cols = NULL;
	cpymo_parser_stream_span output_format;
	output_format.begin = NULL;
	output_format.len = 0;

	size_t input_file_count = 0;
	const char **input_files = (const char **)malloc(argc * sizeof(char **));
	if (input_files == NULL) return CPYMO_ERR_OUT_OF_MEM;

	for (int i = 2; i < argc; ++i) {
		cpymo_parser_stream_span a = cpymo_parser_stream_span_pure(argv[i]);
		cpymo_parser_stream_span_trim(&a);

		if (cpymo_parser_stream_span_equals_str(a, "")) continue;
		else if (cpymo_parser_stream_span_starts_with_str_ignore_case(a, "--")) {
			if (cpymo_parser_stream_span_equals_str_ignore_case(a, "--load-mask"))
				load_mask = true;
			else if (cpymo_parser_stream_span_equals_str_ignore_case(a, "--create-mask"))
				create_mask = true;
			else if (cpymo_parser_stream_span_equals_str_ignore_case(a, "--out-format")) {
				++i;

				if (argc <= i) {
					printf("[Error] --out-format requires an argument.\n");
					help();
					return -1;
				}

				output_format = cpymo_parser_stream_span_pure(argv[i]);
			}
			else {
				printf("[Error] Unknown option: %s\n", argv[i]);
				help();
				return -1;
			}
		}
		else if (output_file == NULL) output_file = argv[i];
		else if (num_of_cols == NULL) num_of_cols = argv[i];
		else input_files[input_file_count++] = argv[i];
	}

	if (output_file == NULL) {
		printf("[Error] You must pass the output file path.\n");
		help();
		return -1;
	}

	if (num_of_cols == NULL) {
		printf("[Error] You must pass the col nums.\n");
		help();
		return -1;
	}

	if (input_file_count < 1) {
		printf("[Error] You must pass at least 1 input file.\n");
		help();
		return -1;
	}

	int num_of_cols_n = atoi(num_of_cols);
	if (num_of_cols_n < 1) {
		printf("[Error] cols must big than 1.\n");
		help();
		return -1;
	}

	if (output_format.begin == NULL) {
		output_format.begin = strrchr(output_file, '.');
		if (output_format.begin) {
			output_format.begin++;
			output_format.len = strlen(output_format.begin);
		}
	}

	error_t err = cpymo_tool_pack_images(
		load_mask,
		create_mask,
		output_file,
		(size_t)num_of_cols_n,
		output_format,
		input_files,
		input_file_count);

	free((void *)input_files);
	
	return process_err(err);
}

