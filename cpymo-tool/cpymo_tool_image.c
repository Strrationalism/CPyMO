#include "cpymo_tool_prelude.h"
#include "cpymo_tool_image.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../cpymo/cpymo_utils.h"
#include "../cpymo/cpymo_gameconfig.h"
#include "../cpymo/cpymo_assetloader.h"
#include "../stb/stb_image_write.h"
#include "../stb/stb_image_resize.h"

error_t cpymo_tool_image_load_from_file(
    cpymo_tool_image *out, const char *filename,
    bool load_mask, const char *mask_format)
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
        error_t err = cpymo_tool_get_mask_name(&mask_file_name, filename, mask_format);
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

error_t cpymo_tool_image_load_from_memory(
    cpymo_tool_image *out, void *memory, size_t len, bool is_mask)
{
    int w, h;
    out->channels = is_mask ? 1 : 4;
    stbi_uc *px = stbi_load_from_memory(memory, (int)len, &w, &h, NULL, out->channels);
    if (px == NULL) return CPYMO_ERR_INVALID_ARG;

    out->width = w;
    out->height = h;
    out->pixels = px;
    return CPYMO_ERR_SUCC;
}

void cpymo_tool_image_attach_mask(cpymo_tool_image *img, const cpymo_tool_image *mask)
{
    cpymo_utils_attach_mask_to_rgba_ex(
        img->pixels,
        img->width,
        img->height,
        mask->pixels,
        mask->width,
        mask->height);
}

error_t cpymo_tool_image_load_attach_mask_from_memory(cpymo_tool_image *img, void *mask_buf, size_t len)
{
    cpymo_tool_image mask;
    error_t err = cpymo_tool_image_load_from_memory(&mask, mask_buf, len, true);
    CPYMO_THROW(err);

    cpymo_tool_image_attach_mask(img, &mask);
    cpymo_tool_image_free(mask);
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

static error_t cpymo_tool_image_create_mask(cpymo_tool_image *out_mask, const cpymo_tool_image *img)
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

static error_t cpymo_tool_image_copy_without_mask(cpymo_tool_image * out_rgb_img, const cpymo_tool_image * img)
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
        out->pixels, (int)w, (int)h, 0, (int)image->channels);

    return CPYMO_ERR_SUCC;
}

void cpymo_tool_image_blit(cpymo_tool_image *dst, const cpymo_tool_image *src, int x, int y)
{
    for (size_t srcy = 0; srcy < src->height; ++srcy) {
        for (size_t srcx = 0; srcx < src->width; ++srcx) {
            int dstx = x + (int)srcx;
            int dsty = y + (int)srcy;

            if (dstx < 0 || dstx >= (int)dst->width || dsty < 0 || dsty >= (int)dst->height)
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

static error_t cpymo_tool_image_save_to_file(
    const cpymo_tool_image *img,
    const char *filename,
    const char *format_cstr)
{
    if (!strcmp(format_cstr, "")) {
        format_cstr = strrchr(filename, '.');
        if (format_cstr) format_cstr++;
    }
    cpymo_str format = cpymo_str_pure(format_cstr);
    int e = 0;
    if (cpymo_str_equals_str_ignore_case(format, "jpg")) {
        e = stbi_write_jpg(filename, (int)img->width, (int)img->height, (int)img->channels, img->pixels, 100);
    }
    else if (cpymo_str_equals_str_ignore_case(format, "bmp")) {
        e = stbi_write_bmp(filename, (int)img->width, (int)img->height, (int)img->channels, img->pixels);
    }
    else if (cpymo_str_equals_str_ignore_case(format, "png")) {
        e = stbi_write_png(filename, (int)img->width, (int)img->height, (int)img->channels, img->pixels, 0);
    }
    else {
        printf("[Error] Unsupported format.\n");
        return CPYMO_ERR_UNSUPPORTED;
    }

    if (e) return CPYMO_ERR_SUCC;
    else return CPYMO_ERR_UNKNOWN;
}

error_t cpymo_tool_image_save_to_file_with_mask(
    const cpymo_tool_image * img,
    const char * filename,
    const char * format,
    bool create_mask,
    const char *mask_format)
{
    if (!create_mask) {
        MASK_FAILED:
        return cpymo_tool_image_save_to_file(img, filename, format);
    }
    else {
        if (mask_format == NULL) mask_format = format;
        cpymo_tool_image mask;
        error_t err = cpymo_tool_image_create_mask(&mask, img);
        if (err != CPYMO_ERR_SUCC) {
            printf("[Warning] Can not create mask: %s(%s).\n", filename, cpymo_error_message(err));
            goto MASK_FAILED;
        }

        char *mask_name = NULL;
        err = cpymo_tool_get_mask_name(&mask_name, filename, mask_format);
        if (err != CPYMO_ERR_SUCC) {
            cpymo_tool_image_free(mask);
            printf("[Warning] Can not get mask name: %s(%s).\n", filename, cpymo_error_message(err));
            goto MASK_FAILED;
        }

        err = cpymo_tool_image_save_to_file(&mask, mask_name, mask_format);
        free(mask_name);
        cpymo_tool_image_free(mask);

        if (err != CPYMO_ERR_SUCC) {
            printf("[Warning] Failed to save mask image: %s(%s).\n", filename, cpymo_error_message(err));
            goto MASK_FAILED;
        }

        cpymo_tool_image rgb;
        err = cpymo_tool_image_copy_without_mask(&rgb, img);
        if (err != CPYMO_ERR_SUCC) goto MASK_FAILED;

        err = cpymo_tool_image_save_to_file(&rgb, filename, format);
        cpymo_tool_image_free(rgb);
        if (err != CPYMO_ERR_SUCC) goto MASK_FAILED;
        return CPYMO_ERR_SUCC;
    }
}

error_t cpymo_tool_image_save_to_memory(const char *format, void **data, size_t *len)
{
    // TODO: Write to memory
    return CPYMO_ERR_UNSUPPORTED;
}

error_t cpymo_tool_get_mask_name_noext(
    char **out_name, const char *assetname)
{
    *out_name = (char *)malloc(strlen(assetname) + 6);
    if (*out_name == NULL) return CPYMO_ERR_OUT_OF_MEM;

    strcpy(*out_name, assetname);
    strcat(*out_name, "_mask");
    return CPYMO_ERR_SUCC;
}

error_t cpymo_tool_get_mask_name(
    char **out_mask_filename, const char *filename, const char *mask_ext)
{
    const char *file_ext = strrchr(filename, '.');
    if (file_ext) file_ext++;
    if (mask_ext == NULL) mask_ext = file_ext;
    else if (!strcmp(mask_ext, "")) mask_ext = file_ext;

    intptr_t filename_no_ext_len = file_ext - filename - 1;
    if (file_ext == NULL) filename_no_ext_len = strlen(filename);
    assert(filename_no_ext_len >= 0);

    *out_mask_filename = (char *)malloc(
        filename_no_ext_len + 7 + (mask_ext == NULL ? 0 : strlen(mask_ext)));
    if (*out_mask_filename == NULL) return CPYMO_ERR_OUT_OF_MEM;

    memcpy(*out_mask_filename, filename, filename_no_ext_len);
    memcpy(*out_mask_filename + filename_no_ext_len, "_mask", 6);
    if (mask_ext) {
        strcat(*out_mask_filename, ".");
        strcat(*out_mask_filename, mask_ext);
    }

    return CPYMO_ERR_SUCC;
}


bool cpymo_backend_image_album_ui_writable(void) { return true; }

static size_t cpymo_tool_generate_album_ui_get_max_page_id(
    cpymo_str album_list_content_text)
{
    size_t max_id = 0;

    cpymo_parser parser;
    cpymo_parser_init(
        &parser, 
        album_list_content_text.begin,
        album_list_content_text.len);
    
    do {
        cpymo_str page_id_str = 
            cpymo_parser_curline_pop_commacell(&parser);
        cpymo_str_trim(&page_id_str);
        if (page_id_str.len == 0) continue;
        size_t page_id = (size_t)cpymo_str_atoi(page_id_str);
        if (page_id > max_id) max_id = page_id;
    } while (cpymo_parser_next_line(&parser));

    return max_id;
}

extern error_t cpymo_album_generate_album_ui_image_pixels(
	void **out_image, 
	cpymo_str album_list_text, 
	cpymo_str output_cache_ui_file_name,
	size_t page, 
	cpymo_assetloader* loader,
	size_t *ref_w, size_t *ref_h);

static void cpymo_tool_generate_album_ui_generate(
    const char *album_list_name,
    bool is_default_album_list,
    cpymo_assetloader *loader)
{
    char *album_list_text = NULL;
    size_t album_list_text_size;
    error_t err = cpymo_assetloader_load_script(
        &album_list_text, &album_list_text_size, album_list_name, loader);

    if (err != CPYMO_ERR_SUCC) {
        if (!(is_default_album_list && err == CPYMO_ERR_SCRIPT_FILE_NOT_FOUND))
            printf(
                "[Error] Can not open album list file \"%s\": %s.\n",
                album_list_name, cpymo_error_message(err));
        return;
    }

    cpymo_str album_list_content = {
        album_list_text,
        album_list_text_size 
    };

    size_t max_page_id = cpymo_tool_generate_album_ui_get_max_page_id(
        album_list_content);

    const size_t game_width = loader->game_config->imagesize_w,
                 game_height = loader->game_config->imagesize_h;

    for (size_t page = 1; page <= max_page_id; ++page) {
        void *pixels = NULL;
        size_t w = game_width, h = game_height;
        err = cpymo_album_generate_album_ui_image_pixels(
            &pixels,
            album_list_content,
            cpymo_str_pure(
                is_default_album_list ? "albumbg" : album_list_name),
            page - 1,
            loader,
            &w, &h);
        free(pixels);
    }

    free(album_list_text);
}

static int cpymo_tool_generate_album_ui(
    const char *gamedir, 
    const char **additional_album_lists,
    size_t additional_album_lists_count)
{
    cpymo_gameconfig gameconfig;
    cpymo_assetloader assetloader;
    {
        char *path = (char *)malloc(strlen(gamedir) + 18);
        if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;
        sprintf(path, "%s/gameconfig.txt", gamedir);
        error_t err = cpymo_gameconfig_parse_from_file(&gameconfig, path);
        if (err != CPYMO_ERR_SUCC) {
            printf("[Error] Can not open file: %s(%s).\n", 
                path, cpymo_error_message(err));
            free(path);
            return err;
        }

        free(path);

        err = cpymo_assetloader_init(&assetloader, &gameconfig, gamedir);
        if (err != CPYMO_ERR_SUCC) {
            printf("[Error] Can not init assetloader: %s %s.\n", 
                path, cpymo_error_message(err));
            return err;
        }
    }

    cpymo_tool_generate_album_ui_generate("album_list", true, &assetloader);
    for (size_t i = 0; i < additional_album_lists_count; ++i) 
        cpymo_tool_generate_album_ui_generate(
            additional_album_lists[i], false, &assetloader);

    cpymo_assetloader_free(&assetloader);
    return CPYMO_ERR_SUCC;
}

extern int help();

int cpymo_tool_invoke_generate_album_ui(
    int argc, const char **argv)
{
    if (argc < 3) return help();
    const char *gamedir = argv[2];

    return cpymo_tool_generate_album_ui(gamedir, argv + 3, (size_t)(argc - 3));
}

