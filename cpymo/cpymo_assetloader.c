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

	char *chbuf = (char *)malloc(gamedir_strlen + 18);
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
		free((void *)loader->gamedir);
	}
}

static error_t cpymo_assetloader_load_filesystem_file(
	char **out_buffer,
	size_t *buf_size,
	const char *asset_type,
	const char *asset_name,
	const char *asset_ext_name,
	const cpymo_assetloader *assetloader) 
{
	char *path = 
		(char *)malloc(
			strlen(assetloader->gamedir) 
			+ strlen(asset_type)
			+ strlen(asset_name)
			+ strlen(asset_ext_name)
			+ 8);

	if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;

	sprintf(
		path, 
		"%s/%s/%s.%s", 
		assetloader->gamedir, 
		asset_type, 
		asset_name, 
		asset_ext_name);

	const error_t err = cpymo_utils_loadfile(path, out_buffer, buf_size);

	free(path);

	return err;
}

static error_t cpymo_assetloader_load_file(
	char **out_buffer,
	size_t *buf_size,
	const char *asset_type,
	const char *asset_name,
	const char *asset_ext_name,
	const bool is_using_pkg,
	const cpymo_package *pkg,
	const cpymo_assetloader *loader)
{
	assert(*out_buffer == NULL);
	if (is_using_pkg) {
		cpymo_package_index index;
		error_t err = cpymo_package_find(&index, pkg, asset_name);
		if (err != CPYMO_ERR_SUCC) return err;

		*buf_size = (size_t)index.file_length;
		*out_buffer = (char *)malloc(*buf_size);
		err = cpymo_package_read_file(*out_buffer, pkg, &index);

		if (err != CPYMO_ERR_SUCC) free(*out_buffer);

		return err;
	}
	else 
		return cpymo_assetloader_load_filesystem_file(
			out_buffer,
			buf_size,
			asset_type,
			asset_name,
			asset_ext_name,
			loader);	
}

error_t cpymo_assetloader_load_bg(char ** out_buffer, size_t * buf_size, const char * bg_name, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_file(
		out_buffer, buf_size, "bg",
		bg_name, loader->game_config->bgformat,
		loader->use_pkg_bg, &loader->pkg_bg,
		loader);
}

error_t cpymo_assetloader_load_bg_image(cpymo_backend_image * img, int * w, int * h, cpymo_parser_stream_span name, const cpymo_assetloader * loader)
{
	char bg_name[40];
	cpymo_parser_stream_span_copy(bg_name, sizeof(bg_name), name);

	char *buf = NULL;
	size_t buf_size = 0;
	error_t err = cpymo_assetloader_load_bg(&buf, &buf_size, bg_name, loader);
	if (err != CPYMO_ERR_SUCC) return err;

	int channels;
	stbi_uc *pixels = stbi_load_from_memory((stbi_uc *)buf, (int)buf_size, w, h, &channels, 3);
	free(buf);

	if (pixels == NULL)
		return CPYMO_ERR_BAD_FILE_FORMAT;

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

error_t cpymo_assetloader_load_chara(char ** out_buffer, size_t * buf_size, const char * chara_name, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_file(
		out_buffer, buf_size, "chara",
		chara_name, loader->game_config->charaformat,
		loader->use_pkg_chara, &loader->pkg_chara,
		loader);
}

error_t cpymo_assetloader_load_chara_mask(char ** out_buffer, size_t * buf_size, const char * chara_name, const cpymo_assetloader * loader)
{
	char filename[64];
	sprintf(filename, "%s_mask", chara_name);

	error_t err = cpymo_assetloader_load_file(
		out_buffer, buf_size, "chara",
		filename, loader->game_config->charamaskformat,
		loader->use_pkg_chara, &loader->pkg_chara,
		loader);

	return err;
}

error_t cpymo_assetloader_load_chara_image(cpymo_backend_image * img, int * w, int * h, cpymo_parser_stream_span name, const cpymo_assetloader * loader)
{
	char *buf = NULL;
	size_t buf_size;

	char chname[48];
	cpymo_parser_stream_span_copy(chname, sizeof(chname), name);

	error_t err = cpymo_assetloader_load_chara(&buf, &buf_size, chname, loader);
	CPYMO_THROW(err);

	int iw, ih, ic;
	stbi_uc *px = stbi_load_from_memory((stbi_uc *)buf, (int)buf_size, &iw, &ih, &ic, 4);
	free(buf);
	buf = NULL;

	if (px == NULL) return CPYMO_ERR_BAD_FILE_FORMAT;

	*w = iw;
	*h = ih;

	if (cpymo_gameconfig_is_symbian(loader->game_config)) {
		err = cpymo_assetloader_load_chara_mask(&buf, &buf_size, chname, loader);

		int mw = 0, mh = 0, mc = 0;
		stbi_uc *mask_px = NULL;

		if (err == CPYMO_ERR_SUCC) {
			mask_px = stbi_load_from_memory((stbi_uc *)buf, (int)buf_size, &mw, &mh, &mc, 1);
			free(buf);
			buf = NULL;
		}

		if (mask_px == NULL) {
			fprintf(stderr, "[Warning] Can not load mask in chara %s.\n", chname);
			return cpymo_backend_image_load(img, px, iw, ih, cpymo_backend_image_format_rgba);
		} 
		else {
			return cpymo_backend_image_load_with_mask(img, px, mask_px, iw, ih, mw, mh);
		}
	}
	else {
		return cpymo_backend_image_load(img, px, iw, ih, cpymo_backend_image_format_rgba);
	}
}

error_t cpymo_assetloader_load_voice(char ** out_buffer, size_t * buf_size, const char * voice_name, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_file(
		out_buffer, buf_size, "voice",
		voice_name, loader->game_config->voiceformat,
		loader->use_pkg_voice, &loader->pkg_voice, 
		loader);
}

error_t cpymo_assetloader_load_se(char ** out_buffer, size_t * buf_size, const char * se_name, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_file(
		out_buffer, buf_size, "se",
		se_name, loader->game_config->seformat,
		loader->use_pkg_se, &loader->pkg_se,
		loader);
}

error_t cpymo_assetloader_load_bgm(char ** out_buffer, size_t * buf_size, const char * bgm_name, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_filesystem_file(
		out_buffer, buf_size, "bgm", bgm_name, 
		loader->game_config->bgmformat, loader);
}

error_t cpymo_assetloader_load_script(char ** out_buffer, size_t * buf_size, const char * script_name, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_filesystem_file(
		out_buffer, buf_size, "script", script_name,
		"txt", loader);
}

error_t cpymo_assetloader_load_video(char ** out_buffer, size_t * buf_size, const char * video_name, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_filesystem_file(
		out_buffer, buf_size, "video", video_name,
		"mp4", loader);
}

error_t cpymo_assetloader_load_system(char ** out_buffer, size_t * buf_size, const char * system_name, const char * ext, const cpymo_assetloader * loader)
{
	return cpymo_assetloader_load_filesystem_file(
		out_buffer, buf_size, "system", system_name,
		ext, loader);
}

error_t cpymo_assetloader_load_system_masktrans(cpymo_backend_masktrans *out, cpymo_parser_stream_span name, const cpymo_assetloader * loader)
{
	char *buf = NULL;
	size_t buf_size;

	char *path = (char *)malloc(name.len + 1);
	if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;
	cpymo_parser_stream_span_copy(path, name.len + 1, name);

	error_t err = cpymo_assetloader_load_system(&buf, &buf_size, path, "png", loader);
	free(path);

	CPYMO_THROW(err);

	int w, h, c;
	stbi_uc *px = stbi_load_from_memory((stbi_uc *)buf, (int)buf_size, &w, &h, &c, 1);
	free(buf);

	if (px == NULL) return CPYMO_ERR_BAD_FILE_FORMAT;

	err = cpymo_backend_masktrans_create(out, px, w, h);
	if (err != CPYMO_ERR_SUCC) {
		free(px);
		return err;
	}

	return CPYMO_ERR_SUCC;
}

error_t cpymo_assetloader_get_bgm_path(char ** out_str, cpymo_parser_stream_span bgm_name, const cpymo_assetloader *loader)
{
	assert(*out_str == NULL);
	char *str = (char *)malloc(
		strlen(loader->gamedir) 
		+ 6 
		+ bgm_name.len
		+ strlen(loader->game_config->bgmformat) 
		+ 4);

	if (str == NULL) return CPYMO_ERR_OUT_OF_MEM;

	strcpy(str, loader->gamedir);
	strcat(str, "/bgm/");
	strncat(str, bgm_name.begin, bgm_name.len);
	strcat(str, ".");
	strcat(str, loader->game_config->bgmformat);
	*out_str = str;

	return CPYMO_ERR_SUCC;
}

error_t cpymo_assetloader_load_system_image(
	cpymo_backend_image * out_image, 
	int *out_width, int *out_height,
	cpymo_parser_stream_span filename_span,
	const char * ext, 
	const cpymo_assetloader * loader,
	bool load_mask)
{
	char filename[128];
	cpymo_parser_stream_span_copy(filename, sizeof(filename) - 10, filename_span);
	char *buf = NULL;
	size_t buf_size = 0;
	error_t err = cpymo_assetloader_load_system(&buf, &buf_size, filename, "png", loader);
	if (err != CPYMO_ERR_SUCC) return err;

	int w, h, c;
	stbi_uc *px = stbi_load_from_memory((stbi_uc *)buf, (int)buf_size, &w, &h, &c, 4);
	free(buf);

	if (px == NULL) return CPYMO_ERR_BAD_FILE_FORMAT;

	cpymo_backend_image img;
	if (load_mask) {
		strcat(filename, "_mask");
		char *mask_buf = NULL; size_t mask_size = 0;
		err = cpymo_assetloader_load_system(&mask_buf, &mask_size, filename, "png", loader);
		if (err != CPYMO_ERR_SUCC) mask_buf = NULL;

		int mw, mh, mc;
		stbi_uc *mask_px = NULL;
		if (err == CPYMO_ERR_SUCC) {
			mask_px = stbi_load_from_memory((stbi_uc *)mask_buf, (int)mask_size, &mw, &mh, &mc, 1);
		}

		if (mask_buf) free(mask_buf);

		if (mask_px) {
			err = cpymo_backend_image_load_with_mask(
				&img, px, mask_px, w, h, mw, mh);
		}
		else {
			goto LOAD_WITHOUT_MASK;
		}
	}
	else {
	LOAD_WITHOUT_MASK:
		err = cpymo_backend_image_load(&img, px, w, h, cpymo_backend_image_format_rgba);
	}

	if (err != CPYMO_ERR_SUCC) {
		free(px);
		return err;
	}

	*out_image = img;
	*out_width = w;
	*out_height = h;
	return CPYMO_ERR_SUCC;
}

