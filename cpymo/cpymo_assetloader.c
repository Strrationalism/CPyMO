#include "cpymo_assetloader.h"
#include "cpymo_utils.h"
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

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
	
	out->gamedir = realloc((void *)out->gamedir, gamedir_strlen + 1);
	
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
			+ 4);

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
	char *filename = (char *)malloc(strlen(chara_name) + 6);
	if (filename == NULL) return CPYMO_ERR_OUT_OF_MEM;

	sprintf(filename, "%s_mask", chara_name);

	error_t err = cpymo_assetloader_load_file(
		out_buffer, buf_size, "chara",
		filename, loader->game_config->charamaskformat,
		loader->use_pkg_chara, &loader->pkg_chara,
		loader);

	free(filename);

	return err;
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

