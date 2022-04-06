#include "cpymo_assetloader.h"
#include "cpymo_utils.h"
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <stb_image.h>

error_t cpymo_assetloader_init(cpymo_assetloader * out, const cpymo_gameconfig * config, const char * gamedir)
{
	error_t err;
	const size_t gamedir_strlen = strlen(gamedir);

	char *chbuf = (char *)malloc(gamedir_strlen + 24);
	out->gamedir = chbuf;

	out->game_config = config;

	if (chbuf == NULL) return CPYMO_ERR_OUT_OF_MEM;

	strcpy(chbuf, gamedir);

	out->use_pkg_bg = false;
	out->use_pkg_chara = false;
	out->use_pkg_se = false;
	out->use_pkg_voice = false;

	// init bg package
	strcpy(chbuf + gamedir_strlen, "/bg/bg.pak");
	err = cpymo_package_open(&out->pkg_bg, chbuf);
	if (err == CPYMO_ERR_SUCC)
		out->use_pkg_bg = true;
	else if (err != CPYMO_ERR_NOT_FOUND && err != CPYMO_ERR_CAN_NOT_OPEN_FILE) {
		cpymo_assetloader_free(out);
		return err;
	}

	// init chara package
	strcpy(chbuf + gamedir_strlen, "/chara/chara.pak");
	err = cpymo_package_open(&out->pkg_chara, chbuf);
	if (err == CPYMO_ERR_SUCC)
		out->use_pkg_chara = true;
	else if (err != CPYMO_ERR_NOT_FOUND && err != CPYMO_ERR_CAN_NOT_OPEN_FILE) {
		cpymo_assetloader_free(out);
		return err;
	}

	// init se package
	strcpy(chbuf + gamedir_strlen, "/se/se.pak");
	err = cpymo_package_open(&out->pkg_se, chbuf);
	if (err == CPYMO_ERR_SUCC)
		out->use_pkg_se = true;
	else if (err != CPYMO_ERR_NOT_FOUND && err != CPYMO_ERR_CAN_NOT_OPEN_FILE) {
		cpymo_assetloader_free(out);
		return err;
	}

	// init voice package
	strcpy(chbuf + gamedir_strlen, "/voice/voice.pak");
	err = cpymo_package_open(&out->pkg_voice, chbuf);
	if (err == CPYMO_ERR_SUCC) 
		out->use_pkg_voice = true;
	else if (err != CPYMO_ERR_NOT_FOUND && err != CPYMO_ERR_CAN_NOT_OPEN_FILE) {
		cpymo_assetloader_free(out);
		return err;
	}

	chbuf[gamedir_strlen] = '\0';
	
	out->gamedir = (char*)realloc((void *)out->gamedir, gamedir_strlen + 1);
	
	return CPYMO_ERR_SUCC;
}

void cpymo_assetloader_free(cpymo_assetloader * loader)
{
	if (loader) {
		if (loader->use_pkg_bg) cpymo_package_close(&loader->pkg_bg);
		if (loader->use_pkg_chara) cpymo_package_close(&loader->pkg_chara);
		if (loader->use_pkg_se) cpymo_package_close(&loader->pkg_se);
		if (loader->use_pkg_voice) cpymo_package_close(&loader->pkg_voice);
		if (loader->gamedir) free((void *)loader->gamedir);
	}
}

static error_t cpymo_assetloader_get_fs_path(
	char **out_str,
	cpymo_parser_stream_span asset_name,
	const char *asset_type,
	const char *asset_ext,
	const cpymo_assetloader *l)
{
	assert(*out_str == NULL);
	char *str = (char *)malloc(
		strlen(l->gamedir)
		+ strlen(asset_type)
		+ 2
		+ asset_name.len
		+ strlen(asset_ext)
		+ 4);

	if (str == NULL) return CPYMO_ERR_OUT_OF_MEM;

	strcpy(str, l->gamedir);
	strcat(str, "/");
	strcat(str, asset_type);
	strcat(str, "/");
	strncat(str, asset_name.begin, asset_name.len);
	strcat(str, ".");
	strcat(str, asset_ext);

	*out_str = str;
	return CPYMO_ERR_SUCC;
}

static error_t cpymo_assetloader_load_filesystem_file(
	char **out_buffer,
	size_t *buf_size,
	const char *asset_type,
	cpymo_parser_stream_span asset_name,
	const char *asset_ext_name,
	const cpymo_assetloader *assetloader) 
{
	char *path = NULL;
	error_t err = cpymo_assetloader_get_fs_path(
		&path, 
		asset_name, 
		asset_type, 
		asset_ext_name, 
		assetloader);
	CPYMO_THROW(err);

	err = cpymo_utils_loadfile(path, out_buffer, buf_size);

	free(path);

	return err;
}

static error_t cpymo_assetloader_load_filesystem_image_pixels(
	void **pixels,
	int *w,
	int *h,
	int c,
	const char *asset_type,
	cpymo_parser_stream_span asset_name,
	const char *asset_ext_name,
	const cpymo_assetloader *l)
{
	char *path = NULL;
	error_t err = cpymo_assetloader_get_fs_path(&path, asset_name, asset_type, asset_ext_name, l);
	CPYMO_THROW(err);
	
	*pixels = stbi_load(path, w, h, NULL, c);
	free(path);

	if (*pixels == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;
	
	return CPYMO_ERR_SUCC;
}

static error_t cpymo_assetloader_load_image_pixels(
	void **pixels,
	int *w,
	int *h,
	int c,
	const char *asset_type,
	cpymo_parser_stream_span asset_name,
	const char *asset_ext_name,
	bool using_pkg,
	const cpymo_package *pkg,
	const cpymo_assetloader *l)
{
	if (using_pkg) 
		return cpymo_package_read_image(pixels, w, h, c, pkg, asset_name);
	else return cpymo_assetloader_load_filesystem_image_pixels(
		pixels,
		w,
		h,
		c,
		asset_type,
		asset_name,
		asset_ext_name,
		l);
}

error_t cpymo_assetloader_load_bg_pixels(void ** px, int * w, int * h, cpymo_parser_stream_span name, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_image_pixels(
		px, w, h, 3, "bg", name,
		loader->game_config->bgformat, loader->use_pkg_bg, &loader->pkg_bg, loader);
}

error_t cpymo_assetloader_load_bg_image(cpymo_backend_image * img, int * w, int * h, cpymo_parser_stream_span name, const cpymo_assetloader * loader)
{
	void *pixels = NULL;
	error_t err = cpymo_assetloader_load_bg_pixels(&pixels, w, h, name, loader);
	CPYMO_THROW(err);

	err = cpymo_backend_image_load(
		img,
		pixels,
		*w,
		*h,
		cpymo_backend_image_format_rgb);

	if (err != CPYMO_ERR_SUCC) {
		free(pixels);
		return err;
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_assetloader_load_image_with_mask(
	cpymo_backend_image *img, int *w, int *h, 
	cpymo_parser_stream_span name, 
	const char *asset_type,
	const char *asset_ext,
	const char *mask_ext,
	bool use_pkg,
	const cpymo_package *pkg,
	const cpymo_assetloader *loader)
{
	void *pixels = NULL;
	error_t err = cpymo_assetloader_load_image_pixels(
		&pixels, w, h, 4, 
		asset_type, name, asset_ext,
		use_pkg, pkg, loader);
	CPYMO_THROW(err);

	if (cpymo_gameconfig_is_symbian(loader->game_config)) {
		char *filename = (char *)malloc(name.len + 6);
		if (filename == NULL) goto LOAD_WITHOUT_MASK;
		
		strncpy(filename, name.begin, name.len);
		strcpy(filename + name.len, "_mask");

		void *mask = NULL;
		int mw, mh;
		err = cpymo_assetloader_load_image_pixels(
			&mask, &mw, &mh, 1,
			asset_type, cpymo_parser_stream_span_pure(filename), mask_ext,
			use_pkg, pkg, loader);
		free(filename);
		if (err == CPYMO_ERR_SUCC) {
			error_t err = cpymo_backend_image_load_with_mask(img, pixels, mask, *w, *h, mw, mh);
			if (err != CPYMO_ERR_SUCC) {
				free(mask);
				goto LOAD_WITHOUT_MASK;
			}
		}
		else goto LOAD_WITHOUT_MASK;
	}
	else {
	LOAD_WITHOUT_MASK:
		err = cpymo_backend_image_load(img, pixels, *w, *h, cpymo_backend_image_format_rgba);
		if (err != CPYMO_ERR_SUCC) free(pixels);
	}

	return CPYMO_ERR_SUCC;
}

error_t cpymo_assetloader_load_chara_image(cpymo_backend_image *img, int *w, int *h, cpymo_parser_stream_span name, const cpymo_assetloader *loader)
{
	return cpymo_assetloader_load_image_with_mask(
		img, w, h,
		name, "chara", loader->game_config->charaformat, loader->game_config->charamaskformat,
		loader->use_pkg_chara, &loader->pkg_chara, loader);
}

error_t cpymo_assetloader_load_script(char ** out_buffer, size_t * buf_size, const char * script_name, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_filesystem_file(
		out_buffer, buf_size, "script", cpymo_parser_stream_span_pure(script_name),
		"txt", loader);
}

error_t cpymo_assetloader_load_system_masktrans(cpymo_backend_masktrans *out, cpymo_parser_stream_span name, const cpymo_assetloader * loader)
{
	void *px = NULL;
	int w, h;
	error_t err = cpymo_assetloader_load_filesystem_image_pixels(&px, &w, &h, 1, "system", name, "png", loader);
	CPYMO_THROW(err);

	err = cpymo_backend_masktrans_create(out, px, w, h);
	if (err != CPYMO_ERR_SUCC) {
		free(px);
		return err;
	}

	return CPYMO_ERR_SUCC;
}

error_t cpymo_assetloader_get_bgm_path(char ** out_str, cpymo_parser_stream_span bgm_name, const cpymo_assetloader *loader)
{
	return cpymo_assetloader_get_fs_path(out_str, bgm_name, "bgm", loader->game_config->bgmformat, loader);
}

error_t cpymo_assetloader_get_vo_path(char **out_str, cpymo_parser_stream_span vo_name, const cpymo_assetloader *l)
{
	return cpymo_assetloader_get_fs_path(out_str, vo_name, "voice", l->game_config->voiceformat, l);
}

error_t cpymo_assetloader_get_video_path(char ** out_str, cpymo_parser_stream_span movie_name, const cpymo_assetloader * l)
{
	return cpymo_assetloader_get_fs_path(out_str, movie_name, "video", "mp4", l);
}

error_t cpymo_assetloader_get_se_path(char **out_str, cpymo_parser_stream_span vo_name, const cpymo_assetloader *l)
{
	return cpymo_assetloader_get_fs_path(out_str, vo_name, "se", l->game_config->seformat, l);
}

error_t cpymo_assetloader_load_system_image(
	cpymo_backend_image * out_image, 
	int *out_width, int *out_height,
	cpymo_parser_stream_span filename_span,
	const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_image_with_mask(
		out_image, out_width, out_height, filename_span, "system", "png", "png", false, NULL, loader);
}

