#include <SDL.h>
#include <stdbool.h>
#include <cpymo_error.h>
#include <cpymo_backend_audio.h>
#include <cpymo_engine.h>
#include <assert.h>

static bool audio_enabled;
extern cpymo_engine engine;

static cpymo_backend_audio_info audio_info = {
	48000,
	cpymo_backend_audio_f32,
	2
};

static void cpymo_backend_audio_sdl_callback(void *userdata, Uint8 * stream, int len)
{
	memset(stream, 0, (size_t)len);
	
	for (size_t cid = 0; cid < CPYMO_AUDIO_MAX_CHANNELS; ++cid) {
		void *samples;
		size_t szlen = (size_t)len;
		size_t written = 0;
		float volume = cpymo_audio_get_channel_volume(cid, &engine.audio);

		while (cpymo_audio_get_samples(&samples, &szlen, cid, &engine.audio) && (size_t)len > written) {
			SDL_MixAudio(stream + written, samples, szlen, (int)(volume * SDL_MIX_MAXVOLUME));
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

void cpymo_backend_audio_init()
{
	SDL_AudioSpec want;
	
	SDL_memset(&want, 0, sizeof(want));
	want.callback = &cpymo_backend_audio_sdl_callback;
	want.channels = 2;
	want.format = AUDIO_F32;
	want.freq = 48000;
	want.samples = 2940;

	SDL_AudioSpec have;
	
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

void cpymo_backend_audio_unlock(void)
{
	SDL_UnlockAudio();
}