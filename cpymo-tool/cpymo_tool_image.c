#include "cpymo_tool_image.h"
#include <cpymo_utils.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stb_image_write.h>
#include <stb_image_resize.h>

error_t cpymo_tool_image_load_from_file(cpymo_tool_image *out, const char *filename, bool load_mask)
{
    int w, h;
    stbi_uc *pixels = stbi_load(filename, &w, &h, NULL, 4);
    if (pixels == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

    out->width = (size_t)w;
    out->height = (size_t)h;
    out->channels = 4;
    out->pixels = pixels;

    if (load_mask) {
        char *mask_file_name = NULL;
        error_t err = cpymo_tool_get_mask_name(&mask_file_name, filename);
        if (err != CPYMO_ERR_SUCC) {
            printf("[Warning] Can not get mask name for image: %s(%s).\n", filename, cpymo_error_message(err));
            return CPYMO_ERR_SUCC;
        }

        int mw, mh;
        stbi_uc *mask = stbi_load(mask_file_name, &mw, &mh, NULL, 1);
        free(mask_file_name);

        if (mask == NULL) {
            printf("[Warning] Can not load mask image for image: %s.\n", filename);
            return CPYMO_ERR_SUCC;
        }

        cpymo_utils_attach_mask_to_rgba_ex(pixels, w, h, mask, mw, mh);
        free(mask);
    }

    return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_image_create(cpymo_tool_image * out, size_t w, size_t h, size_t channels)
{
    stbi_uc *px = (stbi_uc *)malloc(w * h * channels);
    if (px == NULL) return CPYMO_ERR_OUT_OF_MEM;

    out->width = w;
    out->height = h;
    out->channels = channels;
    out->pixels = px;
    return CPYMO_ERR_SUCC;
}

void cpymo_tool_image_free(cpymo_tool_image out)
{
    free(out.pixels);
}

void cpymo_tool_image_fill(cpymo_tool_image *img, uint8_t val)
{
    memset(img->pixels, (int)val, img->width * img->height * img->channels);
}

error_t cpymo_tool_image_create_mask(cpymo_tool_image *out_mask, const cpymo_tool_image *img)
{
    error_t err = cpymo_tool_image_create(out_mask, img->width, img->height, 1);
    CPYMO_THROW(err);

    if (img->channels != 4) {
        cpymo_tool_image_fill(out_mask, 255);
        return CPYMO_ERR_SUCC;
    }

    for (size_t i = 0; i < img->height * img->width; ++i)
        out_mask->pixels[i] = img->pixels[i * 4 + 3];

    return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_image_copy_without_mask(cpymo_tool_image * out_rgb_img, const cpymo_tool_image * img)
{
    if (img->channels != 4 && img->channels != 3) return CPYMO_ERR_UNSUPPORTED;

    error_t err = cpymo_tool_image_create(out_rgb_img, img->width, img->height, 3);
    CPYMO_THROW(err);

    for (size_t i = 0; i < img->height * img->width; ++i) {
        out_rgb_img->pixels[i * 3 + 0] = img->pixels[i * img->channels + 0];
        out_rgb_img->pixels[i * 3 + 1] = img->pixels[i * img->channels + 1];
        out_rgb_img->pixels[i * 3 + 2] = img->pixels[i * img->channels + 2];
    }

    return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_image_resize(cpymo_tool_image *out, const cpymo_tool_image *image, size_t w, size_t h)
{
    error_t err = cpymo_tool_image_create(out, w, h, image->channels);
    CPYMO_THROW(err);

    stbir_resize_uint8(
        image->pixels, (int)image->width, (int)image->height, 0,
        out->pixels, (int)w, (int)h, 0, image->channels);

    return CPYMO_ERR_SUCC;
}

void cpymo_tool_image_blit(cpymo_tool_image *dst, const cpymo_tool_image *src, int x, int y)
{
    for (size_t srcy = 0; srcy < src->height; ++srcy) {
        for (size_t srcx = 0; srcx < src->width; ++srcx) {
            int dstx = x + (int)srcx;
            int dsty = y + (int)srcy;

            if (dstx < 0 || dstx >= dst->width || dsty < 0 || dsty >= dst->height)
                break;

            for (size_t channel = 0; channel < dst->channels; ++channel) {
                uint8_t val = 255;

                if (channel < src->channels)
                    val = src->pixels[srcy * src->width * src->channels + srcx * src->channels + channel];

                dst->pixels[dsty * dst->width * dst->channels + dstx * dst->channels + channel] = val;
            }
        }
    }
}

error_t cpymo_tool_image_save_to_file(const cpymo_tool_image *img, const char *filename, cpymo_parser_stream_span format)
{
    int e = 0;
    if (cpymo_parser_stream_span_equals_str_ignore_case(format, "jpg")) {
        e = stbi_write_jpg(filename, img->width, img->height, img->channels, img->pixels, 100);
    }
    else if (cpymo_parser_stream_span_equals_str_ignore_case(format, "bmp")) {
        e = stbi_write_bmp(filename, img->width, img->height, img->channels, img->pixels);
    }
    else if (cpymo_parser_stream_span_equals_str_ignore_case(format, "png")) {
        e = stbi_write_png(filename, img->width, img->height, img->channels, img->pixels, 0);
    }
    else {
        return CPYMO_ERR_UNSUPPORTED;
    }

    if (e) return CPYMO_ERR_SUCC;
    else return CPYMO_ERR_UNKNOWN;
}

error_t cpymo_tool_get_mask_name(char **out_mask_filename, const char *filename)
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


