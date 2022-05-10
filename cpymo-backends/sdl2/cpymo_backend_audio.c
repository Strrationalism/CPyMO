#include <SDL2/SDL.h>
#include <stdbool.h>
#include <cpymo_error.h>
#include <cpymo_backend_audio.h>
#include <cpymo_engine.h>
#include <assert.h>

#ifdef ENABLE_SDL2_MIXER_AUDIO_BACKEND
#define DISABLE_SDL2_AUDIO_BACKEND
#endif

#ifdef DISABLE_AUDIO
#define DISABLE_SDL2_AUDIO_BACKEND
#endif

#ifndef DISABLE_SDL2_AUDIO_BACKEND
static bool audio_enabled;
extern cpymo_engine engine;

#ifndef SDL2_AUDIO_DEFAULT_FREQ
#define SDL2_AUDIO_DEFAULT_FREQ 48000
#endif

#ifndef SDL2_AUDIO_DEFULAT_CHANNELS
#define SDL2_AUDIO_DEFULAT_CHANNELS 2
#endif

#ifndef SDL2_AUDIO_DEFAULT_FORMAT_SDL
#define SDL2_AUDIO_DEFAULT_FORMAT_SDL AUDIO_F32
#endif

#ifndef SDL2_AUDIO_DEFAULT_FORMAT_CPYMO
#define SDL2_AUDIO_DEFAULT_FORMAT_CPYMO cpymo_backend_audio_f32
#endif

#ifndef SDL2_AUDIO_DEFAULT_SAMPLES
#define SDL2_AUDIO_DEFAULT_SAMPLES 2940
#endif

static cpymo_backend_audio_info audio_info = {
	SDL2_AUDIO_DEFAULT_FREQ,
	SDL2_AUDIO_DEFAULT_FORMAT_CPYMO,
	SDL2_AUDIO_DEFULAT_CHANNELS
};

static void cpymo_backend_audio_sdl_callback(void *userdata, Uint8 * stream, int len)
{
	memset(stream, 0, (size_t)len);
	
	for (size_t cid = 0; cid < CPYMO_AUDIO_MAX_CHANNELS; ++cid) {
		void *samples = NULL;
		size_t szlen = (size_t)len;
		size_t written = 0;
		float volume = cpymo_audio_get_channel_volume(cid, &engine.audio);

		while (cpymo_audio_channel_get_samples(&samples, &szlen, cid, &engine.audio) && szlen) {
			SDL_MixAudio(stream + written, (Uint8 *)samples, (Uint32)szlen, (int)(volume * SDL_MIX_MAXVOLUME));
			written += szlen;
			szlen = (size_t)len - written;
		}
	}
}

const cpymo_backend_audio_info *cpymo_backend_audio_get_info(void)
{
	return audio_enabled ? &audio_info : NULL;
}

static bool cpymo_backend_audio_supported(const SDL_AudioSpec *spec)
{
	return (spec->format == AUDIO_S16SYS
		|| spec->format == AUDIO_S32SYS
		|| spec->format == AUDIO_F32SYS)
		&& spec->padding == 0;
}

static SDL_AudioSpec have;

static uint64_t current_audio_driver;

void cpymo_backend_audio_init()
{
	SDL_AudioSpec want;
	
	SDL_memset(&want, 0, sizeof(want));
	want.callback = &cpymo_backend_audio_sdl_callback;
	want.channels = SDL2_AUDIO_DEFULAT_CHANNELS;
	want.format = SDL2_AUDIO_DEFAULT_FORMAT_SDL;
	want.freq = SDL2_AUDIO_DEFAULT_FREQ;
	want.samples = SDL2_AUDIO_DEFAULT_SAMPLES;
	
	if (SDL_OpenAudio(&want, &have) == 0) {
		if (!cpymo_backend_audio_supported(&have)) {
			SDL_CloseAudio();

			if (SDL_OpenAudio(&want, NULL) == 0) {
				audio_enabled = true;
			}
			else {
				goto FAIL;
			}
		}
		else {
			audio_info.freq = have.freq;
			audio_info.channels = have.channels;

			switch (have.format) {
			case AUDIO_S16SYS:
				audio_info.format = cpymo_backend_audio_s16;
				break;
			case AUDIO_S32SYS:
				audio_info.format = cpymo_backend_audio_s32;
				break;
			case AUDIO_F32SYS:
				audio_info.format = cpymo_backend_audio_f32;
				break;
			default:
				assert(false);
			}

			audio_enabled = true;
		}
	}
	else {
		FAIL:
		audio_enabled = false;
		const char *err = SDL_GetError();
		SDL_Log("[Error] Audio device open failed: %s.", err);
		return;
	}

	SDL_LockAudio();
	SDL_PauseAudio(0);

	current_audio_driver = 
		cpymo_parser_stream_span_hash(
			cpymo_parser_stream_span_pure(
				SDL_GetCurrentAudioDriver()));
}

void cpymo_backend_audio_free()
{
	if (audio_enabled) {
		SDL_CloseAudio();
	}
}

void cpymo_backend_audio_lock(void)
{
	SDL_LockAudio();
}

void cpymo_backend_audio_reset()
{
	if (audio_enabled) {
		uint64_t current_audio_driver_hash =
			cpymo_parser_stream_span_hash(
				cpymo_parser_stream_span_pure(SDL_GetCurrentAudioDriver()));

		if (current_audio_driver == current_audio_driver_hash) return;

		SDL_CloseAudio();

		if (SDL_OpenAudio(&have, NULL) != 0) {
			printf("[Error] Failed to reset audio: %s\n", SDL_GetError());
			audio_enabled = false;
			return;
		}

		SDL_PauseAudio(0);

		current_audio_driver = current_audio_driver_hash;
	}
	else {
		cpymo_backend_audio_init();
		SDL_UnlockAudio();
	}
}

void cpymo_backend_audio_unlock(void)
{
	SDL_UnlockAudio();
}
#else

void cpymo_backend_audio_init() {}
void cpymo_backend_audio_free() {}
void cpymo_backend_audio_reset() {}
const cpymo_backend_audio_info *cpymo_backend_audio_get_info(void) 
{ return NULL; }

#endif