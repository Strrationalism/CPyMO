#include "cpymo_tool_resize.h"
#include <stb_image.h>
#include <stb_image_resize.h>
#include <stb_image_write.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <cpymo_utils.h>
#include <stdint.h>
#include <cpymo_parser.h>

static error_t cpymo_tool_get_mask_name(char **out_mask_filename, const char *filename)
{
    *out_mask_filename = (char *)malloc(strlen(filename) + 1 + strlen("_mask"));
    if (*out_mask_filename == NULL) return CPYMO_ERR_OUT_OF_MEM;
	
    const char *ext = strrchr(filename, '.');
    intptr_t filename_without_ext_len = ext - filename;
	if (ext == NULL) filename_without_ext_len = strlen(filename);
	
    assert(filename_without_ext_len >= 0);

    strcpy(*out_mask_filename, filename);
	strcpy(*out_mask_filename + filename_without_ext_len, "_mask");
	if (ext) strcpy(*out_mask_filename + filename_without_ext_len + 5, ext);

    return CPYMO_ERR_SUCC;
}

typedef struct {
    stbi_uc *main_image;
    stbi_uc *mask_image;
    int main_width;
    int main_height;
    int main_out_channels;
    int mask_width;
    int mask_height;
} cpymo_tool_resize_image_obj;

static error_t cpymo_tool_resize_image_internal(
    cpymo_tool_resize_image_obj *image, 
    double ratio_w, double ratio_h, 
    bool create_mask_image)
{
    if (image->mask_image) {
        cpymo_utils_attach_mask_to_rgba_ex(
            image->main_image, image->main_width, image->main_height,
            image->mask_image, image->mask_width, image->mask_height);
        free(image->mask_image);
        image->mask_image = NULL;
    }

	size_t new_width = (int)(image->main_width * ratio_w);
	size_t new_height = (int)(image->main_height * ratio_h);
	stbi_uc *new_image = (stbi_uc *)malloc(new_width * new_height * 4);
	if (new_image == NULL) return CPYMO_ERR_OUT_OF_MEM;

	stbir_resize_uint8(
        image->main_image, image->main_width, image->main_height, 0,
        new_image, (int)new_width, (int)new_height, 0, 4);
	
    free(image->main_image);
    image->main_image = new_image;

    image->main_width = (int)new_width;
    image->main_height = (int)new_height;
    image->main_out_channels = 4;
	
    if (create_mask_image) {
		image->mask_width = (int)new_width;
		image->mask_height = (int)new_height;
		image->mask_image = (stbi_uc *)malloc(new_width * new_height);
		if (image->mask_image == NULL) return CPYMO_ERR_OUT_OF_MEM;

        for (size_t i = 0; i < new_width * new_height; i++) {
            image->mask_image[i] = image->main_image[i * 4 + 3];
        }

        new_image = (stbi_uc *)malloc(new_width * new_height * 3);
        if (new_image) {
            for (size_t i = 0; i < new_width * new_height; i++) {
				new_image[i * 3 + 0] = image->main_image[i * 4 + 0];
                new_image[i * 3 + 1] = image->main_image[i * 4 + 1];
                new_image[i * 3 + 2] = image->main_image[i * 4 + 2];
            }
			
            image->main_out_channels = 3;			
			free(image->main_image);
            image->main_image = new_image;
        }
    }

    return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_resize_image(
    const char *input_file, 
    const char *output_file, 
    double ratio_w, 
    double ratio_h, 
    bool load_mask, 
    bool create_mask, 
    const char * out_format)
{
    cpymo_tool_resize_image_obj image;
    image.mask_image = NULL;
	image.main_image = stbi_load(input_file, &image.main_width, &image.main_height, NULL, 4);
    if (image.main_image == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

    error_t err = CPYMO_ERR_SUCC;

    if (load_mask) {
        char *mask_filename;
        err = cpymo_tool_get_mask_name(&mask_filename, input_file);
        if (err != CPYMO_ERR_SUCC) goto CLEAN;
		
		image.mask_image = stbi_load(mask_filename, &image.mask_width, &image.mask_height, NULL, 1);
        if (image.mask_image == NULL) {
            printf("[Warning] Can not open mask image file %s for %s.\n", mask_filename, input_file);
        }

		free(mask_filename);
    }

    err = cpymo_tool_resize_image_internal(&image, ratio_w, ratio_h, create_mask);
    if (err != CPYMO_ERR_SUCC) goto CLEAN;
	
    cpymo_parser_stream_span out_format_span = cpymo_parser_stream_span_pure(out_format);
    if (cpymo_parser_stream_span_equals_str_ignore_case(out_format_span, "jpg")) {
        stbi_write_jpg(output_file, image.main_width, image.main_height, image.main_out_channels, image.main_image, 100);			
    }
    else if (cpymo_parser_stream_span_equals_str_ignore_case(out_format_span, "bmp")) {
        stbi_write_bmp(output_file, image.main_width, image.main_height, image.main_out_channels, image.main_image);
    }
    else if(cpymo_parser_stream_span_equals_str_ignore_case(out_format_span, "png")) {
		stbi_write_png(output_file, image.main_width, image.main_height, image.main_out_channels, image.main_image, 0);
	}
	else {
		printf("[Error] Unsupported output format %s.\n", out_format);
		err = CPYMO_ERR_UNSUPPORTED;
        goto CLEAN;
	}

    if (image.mask_image) {
        char *out_mask;
        err = cpymo_tool_get_mask_name(&out_mask, output_file);
        if (err != CPYMO_ERR_SUCC) {
            printf("[Error] No memory for create mask file.\n");
			goto CLEAN;
        }

		if (cpymo_parser_stream_span_equals_str_ignore_case(out_format_span, "jpg")) {
			stbi_write_jpg(out_mask, image.mask_width, image.mask_height, 1, image.mask_image, 100);			
		}
		else if (cpymo_parser_stream_span_equals_str_ignore_case(out_format_span, "bmp")) {
			stbi_write_bmp(out_mask, image.mask_width, image.mask_height, 1, image.mask_image);
		}
        else if (cpymo_parser_stream_span_equals_str_ignore_case(out_format_span, "png")) {
            stbi_write_png(out_mask, image.mask_width, image.mask_height, 1, image.mask_image, 0);
        }
        else {
			printf("[Error] Unsupported output format %s.\n", out_format);
			err = CPYMO_ERR_UNSUPPORTED;
        }
		
        free(out_mask);
    }
	
CLEAN:
    if (image.mask_image) free(image.mask_image);
    if (image.main_image) free(image.main_image);
    return err;
}

error_t cpymo_tool_resize_image_package(
    const char *input_file, const char *output_file,
    double ratio_w, double ratio_h,
    bool load_mask, bool create_mask,
    const char *out_format)
{
    return CPYMO_ERR_NOT_FOUND;
}