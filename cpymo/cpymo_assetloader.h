#ifndef INCLUDE_CPYMO_ASSETLOADER
#define INCLUDE_CPYMO_ASSETLOADER

#include "cpymo_package.h"
#include "cpymo_gameconfig.h"
#include <stddef.h>

typedef struct {
	bool use_pkg_bg, use_pkg_chara, use_pkg_se, use_pkg_voice;
	cpymo_package pkg_bg, pkg_chara, pkg_se, pkg_voice;
	const cpymo_gameconfig *game_config;
	const char *gamedir;
} cpymo_assetloader;

error_t cpymo_assetloader_init(cpymo_assetloader *out, cpymo_gameconfig *config, const char *gamedir);
void cpymo_assetloader_free(cpymo_assetloader *loader);

error_t cpymo_assetloader_load_bg(char **out_buffer, size_t *buf_size, const char *bg_name, const cpymo_assetloader *loader);
error_t cpymo_assetloader_load_chara(char **out_buffer, size_t *buf_size, const char *chara_name, const cpymo_assetloader *loader);
error_t cpymo_assetloader_load_chara_mask(char **out_buffer, size_t *buf_size, const char *chara_name, const cpymo_assetloader *loader);
error_t cpymo_assetloader_load_voice(char **out_buffer, size_t *buf_size, const char *voice_name, const cpymo_assetloader *loader);
error_t cpymo_assetloader_load_se(char **out_buffer, size_t *buf_size, const char *se_name, const cpymo_assetloader *loader);
error_t cpymo_assetloader_load_bgm(char **out_buffer, size_t *buf_size, const char *bgm_name, const cpymo_assetloader *loader);
error_t cpymo_assetloader_load_script(char **out_buffer, size_t *buf_size, const char *script_name, const cpymo_assetloader *loader);
error_t cpymo_assetloader_load_video(char **out_buffer, size_t *buf_size, const char *video_name, const cpymo_assetloader *loader);
error_t cpymo_assetloader_load_system(char **out_buffer, size_t *buf_size, const char *asset_name, const char *ext, const cpymo_assetloader *loader);

#endif