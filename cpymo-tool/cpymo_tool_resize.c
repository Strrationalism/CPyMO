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
#include "cpymo_tool_image.h"

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
    assert(image->mask_image == NULL);

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
    cpymo_tool_image img;

    error_t err = cpymo_tool_image_load_from_file(&img, input_file, load_mask);
    CPYMO_THROW(err);

    image.main_image = img.pixels;
    image.mask_image = NULL;
    image.main_height = img.height;
    image.main_width = img.width;

    err = cpymo_tool_resize_image_internal(&image, ratio_w, ratio_h, create_mask);
    if (err != CPYMO_ERR_SUCC) goto CLEAN;
	
    cpymo_parser_stream_span out_format_span = cpymo_parser_stream_span_pure(out_format);
    img.channels = image.main_out_channels;
    img.width = image.main_width;
    img.height = image.main_height;
    img.pixels = image.main_image;
    image.main_image = NULL;
    err = cpymo_tool_image_save_to_file(&img, output_file, out_format_span);
    if (err != CPYMO_ERR_SUCC) {
        printf("[Error] Can not save image: %s(%s).\n", output_file, cpymo_error_message(err));
        goto CLEAN;
    }

    cpymo_tool_image_free(img);

    if (image.mask_image) {
        char *out_mask;
        err = cpymo_tool_get_mask_name(&out_mask, output_file);
        if (err != CPYMO_ERR_SUCC) {
            printf("[Error] No memory for create mask file.\n");
			goto CLEAN;
        }

        img.channels = 1;
        img.width = image.mask_width;
        img.height = image.mask_height;
        img.pixels = image.mask_image;
        image.mask_image = NULL;

        err = cpymo_tool_image_save_to_file(&img, out_mask, out_format_span);
        free(out_mask);
        cpymo_tool_image_free(img);

        if (err != CPYMO_ERR_SUCC) {
            printf("[Warning] Can not save mask image: %s(%s).\n", output_file, cpymo_error_message(err));
        }
    }
	
CLEAN:
    if (image.mask_image) free(image.mask_image);
    if (image.main_image) free(image.main_image);
    return err;
}
