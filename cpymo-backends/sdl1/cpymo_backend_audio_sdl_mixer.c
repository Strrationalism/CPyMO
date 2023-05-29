#include "../../cpymo/cpymo_prelude.h"
#ifdef ENABLE_SDL_MIXER_AUDIO_BACKEND
#include "../../cpymo/cpymo_engine.h"
#include <SDL/SDL_mixer.h>
#include <assert.h>


#ifndef SDL_MIXER_FREQ
#define SDL_MIXER_FREQ 22050
#endif

#ifndef SDL_MIXER_CHANNELS
#define SDL_MIXER_CHANNELS 2
#endif

#ifndef SDL_MIXER_CHUNKSIZE
#define SDL_MIXER_CHUNKSIZE 4096
#endif

static bool enabled = false;

void cpymo_audio_init(cpymo_audio_system *s)
{
    Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_MOD | MIX_INIT_FLAC);

    if (Mix_OpenAudio(
            SDL_MIXER_FREQ,
            MIX_DEFAULT_FORMAT, 
            SDL_MIXER_CHANNELS,
            SDL_MIXER_CHUNKSIZE) == -1)
        return;

    Mix_AllocateChannels(2);

    enabled = true;
}

void cpymo_audio_free(cpymo_audio_system *s)
{
    Mix_CloseAudio();
    Mix_Quit();
}

float cpymo_audio_get_channel_volume(size_t cid, const cpymo_audio_system *s)
{
    if (!enabled)
        return 0;
    else if (cid == CPYMO_AUDIO_CHANNEL_BGM)
        return (float)Mix_VolumeMusic(-1) / (float)MIX_MAX_VOLUME;
    else 
        return (float)Mix_Volume(cid - 1, -1) / (float)MIX_MAX_VOLUME;
}

void cpymo_audio_set_channel_volume(size_t cid, cpymo_audio_system *s, float vol_)
{
    if (!enabled) return;
    int vol = (int)(vol_ * MIX_MAX_VOLUME);

    if (cid == CPYMO_AUDIO_CHANNEL_BGM)
        Mix_VolumeMusic(vol);
    else 
        Mix_Volume(cid - 1, vol);
}
bool cpymo_audio_channel_is_playing(size_t cid, cpymo_audio_system *s)
{
#ifndef DISABLE_SDL2_MIXER_MUSIC
	if (cid == CPYMO_AUDIO_CHANNEL_BGM)
		return Mix_PlayingMusic();
#endif
	return Mix_Playing((int)(cid - 1));
}

bool cpymo_audio_channel_is_looping(size_t cid, cpymo_audio_system *s)
{
	return false;
}

bool cpymo_audio_enabled(struct cpymo_engine *e)
{
    return enabled;
}

static bool se_looping = false;
bool cpymo_audio_wait_se(struct cpymo_engine *e, float d)
{
	if (!enabled) return true;
	if (Mix_Playing(CPYMO_AUDIO_CHANNEL_SE - 1)) {
		if (se_looping) {
			printf("[Error] Can not wait for a looping SE.\n");
			return true;
		}
		else return false;
	}
	else return true;
}

static Mix_Music *bgm = NULL;
static char *bgm_name = NULL;
error_t cpymo_audio_bgm_play(cpymo_engine *e, cpymo_str bgmname, bool loop)
{
    if (!enabled) return CPYMO_ERR_SUCC;

    cpymo_audio_bgm_stop(e);

    char *bgm_path = NULL;
    error_t err = cpymo_assetloader_get_bgm_path(&bgm_path, bgmname, &e->assetloader);
    CPYMO_THROW(err);

    bgm = Mix_LoadMUS(bgm_path);
    if (bgm == NULL) {
        printf("[Warning] Can not open %s: %s\n", bgm_path, Mix_GetError());
        free(bgm_path);
        return CPYMO_ERR_CAN_NOT_OPEN_FILE;
    }

    free(bgm_path);

    Mix_PlayMusic(bgm, loop ? -1 : 0);

    if (loop) {
        assert(bgm_name == NULL);
        bgm_name = (char *)malloc(bgmname.len + 1);
        if (bgm_name)
            cpymo_str_copy(bgm_name, bgmname.len + 1, bgmname);
    }

    return CPYMO_ERR_SUCC;
}

void cpymo_audio_bgm_stop(struct cpymo_engine *e)
{
    if (bgm) {
        Mix_HaltMusic();
        Mix_FreeMusic(bgm);
        bgm = NULL;
    }

    if (bgm_name) {
        free(bgm_name);
        bgm_name = NULL;
    }
}

static char *se_data = NULL;
static SDL_RWops *se_rwops = NULL;
static Mix_Chunk *se = NULL;
static char *se_name = NULL;
error_t cpymo_audio_se_play(struct cpymo_engine *e, cpymo_str sename, bool loop)
{
    if (!enabled) return CPYMO_ERR_SUCC;
    cpymo_audio_se_stop(e);

    if (e->assetloader.use_pkg_se) {
        size_t sz;
        error_t err = cpymo_package_read_file(&se_data, &sz, &e->assetloader.pkg_se, sename);
        CPYMO_THROW(err);

        se_rwops = SDL_RWFromConstMem(se_data, sz);
        if (se_rwops == NULL) {
            free(se_data);
            se_data = NULL;
            return CPYMO_ERR_UNKNOWN;
        }

        se = Mix_LoadWAV_RW(se_rwops, 0);
        if (se == NULL) {
            printf("[Warning] Can not load se: %s\n", Mix_GetError());
            SDL_FreeRW(se_rwops);
            se_rwops = NULL;
            free(se_data);
            se_data = NULL;
            return CPYMO_ERR_UNKNOWN;
        }
    }
    else {
        char *se_path = NULL;
        error_t err = cpymo_assetloader_get_se_path(&se_path, sename, &e->assetloader);
        CPYMO_THROW(err);

        se = Mix_LoadWAV(se_path);
        if (se == NULL) {
            printf("[Warning] Can not open %s: %s\n", se_path, Mix_GetError());
            free(se_path);
            return CPYMO_ERR_CAN_NOT_OPEN_FILE;
        }

        free(se_path);
    }

    Mix_PlayChannel(CPYMO_AUDIO_CHANNEL_SE - 1, se, loop ? -1 : 0);

    if (loop) {
        assert(se_name == NULL);
        se_name = (char *)malloc(sename.len + 1);
        if (se_name) {
            cpymo_str_copy(se_name, sename.len + 1, sename);
        }
    }

	return CPYMO_ERR_SUCC;
}

void cpymo_audio_se_stop(struct cpymo_engine *e)
{
    if (se) {
        Mix_HaltChannel(CPYMO_AUDIO_CHANNEL_SE - 1);
        Mix_FreeChunk(se);
        se = NULL;
    }

    if (se_rwops) {
        SDL_FreeRW(se_rwops);
        se_rwops = NULL;
    }

    if (se_data) {
        free(se_data);
        se_data = NULL;
    }

    if (se_name) {
        free(se_name);
        se_name = NULL;
    }
}

static char *vo_data = NULL;
static SDL_RWops *vo_rwops = NULL;
static Mix_Chunk *vo = NULL;
error_t cpymo_audio_vo_play(struct cpymo_engine *e, cpymo_str voname)
{
    if (!enabled) return CPYMO_ERR_SUCC;
    cpymo_audio_vo_stop(e);

    if (e->assetloader.use_pkg_voice) {
        size_t sz;
        error_t err = cpymo_package_read_file(&vo_data, &sz, &e->assetloader.pkg_voice, voname);
        CPYMO_THROW(err);

        vo_rwops = SDL_RWFromConstMem(vo_data, sz);
        if (vo_rwops == NULL) {
            free(vo_data);
            vo_data = NULL;
            return CPYMO_ERR_UNKNOWN;
        }

        vo = Mix_LoadWAV_RW(vo_rwops, 0);
        if (vo == NULL) {
            SDL_FreeRW(vo_rwops);
            free(vo_data);
            vo_rwops = NULL;
            vo_data = NULL;

            printf("[Warning] Can not open voice: %s\n", Mix_GetError());

            return CPYMO_ERR_UNKNOWN;
        }
    }
    else {
        char *path = NULL;
        error_t err = cpymo_assetloader_get_vo_path(&path, voname, &e->assetloader);
        CPYMO_THROW(err);

        vo = Mix_LoadWAV(path);
        free(path);

        if (vo == NULL) {
            printf("[Warning] Can not open voice: %s\n", Mix_GetError());
            return CPYMO_ERR_UNKNOWN;
        }
    }

    Mix_PlayChannel(CPYMO_AUDIO_CHANNEL_VO - 1, vo, 0);

    return CPYMO_ERR_SUCC;
}

void cpymo_audio_vo_stop(struct cpymo_engine *e)
{
    if (vo) {
        Mix_HaltChannel(CPYMO_AUDIO_CHANNEL_VO - 1);
        Mix_FreeChunk(vo);
        vo = NULL;
    }

    if (vo_rwops) {
        SDL_FreeRW(vo_rwops);
        vo_rwops = NULL;
    }

    if (vo_data) {
        free(vo_data);
        vo_data = NULL;
    }
}

const char *cpymo_audio_get_bgm_name(struct cpymo_engine *e)
{
    return bgm_name ? bgm_name : "";
}

const char *cpymo_audio_get_se_name(struct cpymo_engine *e)
{
	return "";
}

#endif
