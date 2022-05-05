#ifdef ENABLE_SDL2_MIXER_AUDIO_BACKEND
#include <SDL.h>
#include <SDL_mixer.h>
#include <cpymo_audio.h>
#include <cpymo_engine.h>

static bool enabled = false;

static float volumes[CPYMO_AUDIO_MAX_CHANNELS];

static bool se_looping = false;

static char *bgm_name = NULL;
static char *se_name = NULL;

#ifdef DISABLE_SDL2_MIXER_MUSIC
static Mix_Chunk *bgm = NULL;
#define CHUNK_ID(x) x
#else
static Mix_Music *bgm = NULL;
#define CHUNK_ID(x) (x - 1)
#endif

static Mix_Chunk *se = NULL;
static Mix_Chunk *vo = NULL;

static void *se_data = NULL;
static void *vo_data = NULL;
static SDL_RWops *se_rwops = NULL;
static SDL_RWops *vo_rwops = NULL;

#ifndef SDL2_MIXER_AUDIO_BACKEND_FREQ
#define SDL2_MIXER_AUDIO_BACKEND_FREQ 44100
#endif

#ifdef __ANDROID__
#define SDL2_MIXER_AUDIO_BACKEND_CHUNK_SIZE (1024 * 1024 * 8)
#endif

#ifndef SDL2_MIXER_AUDIO_BACKEND_CHUNK_SIZE
#define SDL2_MIXER_AUDIO_BACKEND_CHUNK_SIZE (1024 * 256)
#endif

void cpymo_audio_init(cpymo_audio_system *s)
{
	enabled = false;
	bgm_name = NULL;
	se_name = NULL;
	se_looping = false;
	bgm = NULL;
	se = NULL;
	vo = NULL;
	se_data = NULL;
	se_rwops = NULL;
	vo_data = NULL;
	vo_rwops = NULL;
	
	for (size_t i = 0; i < CPYMO_AUDIO_MAX_CHANNELS; ++i) {
		volumes[i] = 1;
	}
	
	int supported = Mix_Init(
			MIX_INIT_FLAC |
			MIX_INIT_MP3 |
			MIX_INIT_OGG |
			MIX_INIT_MID |
			MIX_INIT_MOD);

	if ((supported & MIX_INIT_MP3) == 0)
		SDL_Log("[Warning] MP3 not supported.\n");

	if ((supported & MIX_INIT_OGG) == 0)
		SDL_Log("[Warning] OGG not supported.\n");

	if (Mix_OpenAudio(SDL2_MIXER_AUDIO_BACKEND_FREQ, MIX_DEFAULT_FORMAT, 2, SDL2_MIXER_AUDIO_BACKEND_CHUNK_SIZE) == -1) {
        SDL_Log("[Error] SDL2_Mixer open audio failed: %s\n", Mix_GetError());
		Mix_Quit();
		return;
	}

	Mix_AllocateChannels(CHUNK_ID(CPYMO_AUDIO_MAX_CHANNELS));
	enabled = true;
}

void cpymo_audio_free(cpymo_audio_system *s)
{
	if (enabled) {
		cpymo_audio_bgm_stop(NULL);
		cpymo_audio_se_stop(NULL);
		cpymo_audio_vo_stop(NULL);
		Mix_AllocateChannels(0);
		Mix_CloseAudio();
		Mix_Quit();
	}
}

float cpymo_audio_get_channel_volume(size_t cid, const cpymo_audio_system *s)
{
	return volumes[cid];
}

void cpymo_audio_set_channel_volume(size_t cid, cpymo_audio_system *s, float vol)
{
	if (enabled) {
		volumes[cid] = vol;
#ifdef DISABLE_SDL2_MIXER_MUSIC
        Mix_Volume((int)CHUNK_ID(cid), (int)(vol * MIX_MAX_VOLUME));
#else
		if (cid == CPYMO_AUDIO_CHANNEL_BGM)
			Mix_VolumeMusic((int)(vol * MIX_MAX_VOLUME));
		else {
			Mix_Volume((int)CHUNK_ID(cid), (int)(vol * MIX_MAX_VOLUME));
		}
#endif
	}
}

bool cpymo_audio_enabled(struct cpymo_engine *e)
{
	return enabled;
}

bool cpymo_audio_wait_se(struct cpymo_engine *e, float d)
{
	if (!enabled) return false;
	if (Mix_Playing(CHUNK_ID(CPYMO_AUDIO_CHANNEL_SE))) {
		if (se_looping) {
			SDL_Log("[Error] Can not wait for a looping SE.\n");
			return true;
		}
		else return false;
	}
	else return true;
}

error_t cpymo_audio_bgm_play(cpymo_engine *e, cpymo_parser_stream_span bgmname, bool loop)
{
	if (!enabled) return CPYMO_ERR_SUCC;
	cpymo_audio_bgm_stop(e);

	char *path = NULL;
	error_t err = cpymo_assetloader_get_bgm_path(&path, bgmname, &e->assetloader);
	CPYMO_THROW(err);

#ifdef DISABLE_SDL2_MIXER_MUSIC
	bgm = Mix_LoadWAV(path);
#else
	bgm = Mix_LoadMUS(path);
#endif

    free(path);

	if (bgm == NULL) {
		SDL_Log("[Error] Can not load music: %s\n", Mix_GetError());
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}		
#ifdef DISABLE_SDL2_MIXER_MUSIC
	int err2 = Mix_PlayChannel(CHUNK_ID(CPYMO_AUDIO_CHANNEL_BGM), bgm, loop ? -1 : 0);
#else
	int err2 = Mix_PlayMusic(bgm, loop ? -1 : 1);
#endif

	if (err2 == -1) {
		SDL_Log("[Error] Can not play music: %s\n", Mix_GetError());
		return CPYMO_ERR_UNKNOWN;
	}
	
	if (loop) {
		bgm_name = (char *)malloc(bgmname.len + 1);
		if (bgm_name)
			cpymo_parser_stream_span_copy(bgm_name, bgmname.len + 1, bgmname);
	}

	return CPYMO_ERR_SUCC;
}

void cpymo_audio_bgm_stop(struct cpymo_engine *e)
{
	if (!enabled) return;
#ifdef DISABLE_SDL2_MIXER_MUSIC
    Mix_HaltChannel(CHUNK_ID(CPYMO_AUDIO_CHANNEL_BGM));
    if (bgm) Mix_FreeChunk(bgm);
#else
	Mix_HaltMusic();
	if (bgm) Mix_FreeMusic(bgm);
#endif
	bgm = NULL;
	if (bgm_name) free(bgm_name);
	bgm_name = NULL;
}

error_t cpymo_audio_se_play(struct cpymo_engine *e, cpymo_parser_stream_span sename, bool loop)
{
	if (!enabled) return CPYMO_ERR_SUCC;
#ifdef __PSP__
	return CPYMO_ERR_SUCC;
#endif

	cpymo_audio_se_stop(e);

	if (e->assetloader.use_pkg_se) {
		cpymo_package_index index;
		error_t err = cpymo_package_find(&index, &e->assetloader.pkg_se, sename);
		CPYMO_THROW(err);

		se_data = malloc(index.file_length);
		if (se_data == NULL) return CPYMO_ERR_OUT_OF_MEM;

		err = cpymo_package_read_file_from_index(
			(char *)se_data, &e->assetloader.pkg_se, &index);
		if (err != CPYMO_ERR_SUCC) {
			free(se_data);
			se_data = NULL;
			return err;
		}

		se_rwops = SDL_RWFromConstMem(se_data, index.file_length);
		if (se_rwops == NULL) {
			free(se_data);
			se_data = NULL;
			return CPYMO_ERR_UNKNOWN;
		}
	}
	else {
		char *path = NULL;
		error_t err = cpymo_assetloader_get_se_path(&path, sename, &e->assetloader);
		CPYMO_THROW(err);

		se_rwops = SDL_RWFromFile(path, "rb");
		free(path);

		if (se_rwops == NULL) {
			return CPYMO_ERR_CAN_NOT_OPEN_FILE;
		}
	}

	se = Mix_LoadWAV_RW(se_rwops, 0);
	if (se == NULL) {
		SDL_RWclose(se_rwops);
		se_rwops = NULL;
		if (se_data) free(se_data);
		se_data = NULL;
		SDL_Log("[Error] Can not load SE: %s\n", Mix_GetError());
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	Mix_PlayChannel(CHUNK_ID(CPYMO_AUDIO_CHANNEL_SE), se, loop ? -1 : 0);

	if (loop) {
		se_name = (char *)malloc(sename.len + 1);
		if (se_name)
			cpymo_parser_stream_span_copy(se_name, sename.len + 1, sename);
	}

	se_looping = loop;

	return CPYMO_ERR_SUCC;
}

void cpymo_audio_se_stop(struct cpymo_engine *e)
{
    se_looping = false;
	if (!enabled) return;
	Mix_HaltChannel(CHUNK_ID(CPYMO_AUDIO_CHANNEL_SE));
	if (se) Mix_FreeChunk(se);
	se = NULL;
	if (se_name) free(se_name);
	se_name = NULL;
	if (se_rwops) SDL_RWclose(se_rwops);
	se_rwops = NULL;
	if (se_data) free(se_data);
	se_data = NULL;
}

error_t cpymo_audio_vo_play(struct cpymo_engine *e, cpymo_parser_stream_span voname)
{
	if (!enabled) return CPYMO_ERR_SUCC;
#ifdef __PSP__
	return CPYMO_ERR_SUCC;
#endif
	cpymo_audio_vo_stop(e);

	if (e->assetloader.use_pkg_voice) {
		cpymo_package_index index;
		error_t err = cpymo_package_find(&index, &e->assetloader.pkg_voice, voname);
		CPYMO_THROW(err);

		vo_data = malloc(index.file_length);
		if (vo_data == NULL) return CPYMO_ERR_OUT_OF_MEM;
		
		err = cpymo_package_read_file_from_index(
			(char *)vo_data, &e->assetloader.pkg_voice, &index);
		
		if (err != CPYMO_ERR_SUCC) {
			free(vo_data);
			vo_data = NULL;
			return err;
		}
		
		vo_rwops = SDL_RWFromConstMem(vo_data, (int)index.file_length);
		if (vo_rwops == NULL) {
			free(vo_data);
			vo_data = NULL;
			return CPYMO_ERR_UNKNOWN;
		}
	}
	else {
		char *path = NULL;
		error_t err = cpymo_assetloader_get_vo_path(&path, voname, &e->assetloader);
		CPYMO_THROW(err);
		
		vo_rwops = SDL_RWFromFile(path, "rb");
		free(path);

		if (vo_rwops == NULL) {
			return CPYMO_ERR_CAN_NOT_OPEN_FILE;
		}
	}

	vo = Mix_LoadWAV_RW(vo_rwops, 0);
	if (vo == NULL) {
		SDL_Log("[Error] Can not load vo chunk: %s\n", Mix_GetError());
		SDL_RWclose(vo_rwops);
		vo_rwops = NULL;
		if (vo_data) free(vo_data);
		vo_data = NULL;
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}
	
	Mix_PlayChannel(CHUNK_ID(CPYMO_AUDIO_CHANNEL_VO), vo, 0);
	
	return CPYMO_ERR_SUCC;
}

void cpymo_audio_vo_stop(struct cpymo_engine *e)
{
	if (!enabled) return;
	Mix_HaltChannel(CHUNK_ID(CPYMO_AUDIO_CHANNEL_VO));
	if (vo) Mix_FreeChunk(vo);
	vo = NULL;
	if (vo_rwops) SDL_RWclose(vo_rwops);
	vo_rwops = NULL;
	if (vo_data) free(vo_data);
	vo_data = NULL;
}

const char *cpymo_audio_get_bgm_name(struct cpymo_engine *e)
{
	if (!enabled) return "";
	return bgm_name;
}

const char *cpymo_audio_get_se_name(struct cpymo_engine *e)
{
	if (!enabled) return "";
	return se_name;
}

#endif