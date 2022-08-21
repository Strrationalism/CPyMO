#include "cpymo_prelude.h"
#include <cpymo_engine.h>
#ifndef DISABLE_FFMPEG_AUDIO

#include <SDL/SDL.h>
#include <cpymo_backend_audio.h>

#ifndef DEFAULT_CHANNELS
#define DEFAULT_CHANNELS 2
#endif

#ifndef DEFAULT_FREQ
#define DEFAULT_FREQ 22050
#endif

#ifndef DEFAULT_SAMPLES
#define DEFAULT_SAMPLES 8192
#endif

extern cpymo_engine engine;
static cpymo_backend_audio_info info;
static bool enabled = false;

const cpymo_backend_audio_info *cpymo_backend_audio_get_info(void)
{
    return enabled ? &info : NULL;
}

static void cpymo_backend_audio_callback(void *userdata, Uint8 *stream, int len)
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

static inline bool cpymo_backend_audio_supported(const SDL_AudioSpec *spec)
{
    return spec->format == AUDIO_S16SYS && spec->padding == 0;
}

void cpymo_backend_audio_init(void)
{
    SDL_AudioSpec want, got;
    want.callback = &cpymo_backend_audio_callback;
    want.channels = DEFAULT_CHANNELS;
    want.format = AUDIO_S16SYS;
    want.freq = DEFAULT_FREQ;
    want.padding = 0;
    want.samples = DEFAULT_SAMPLES;
    want.silence = 0;
    want.size = 0;
    want.userdata = NULL;

    if (SDL_OpenAudio(&want, &got) == 0) {
        if (!cpymo_backend_audio_supported(&got)) {
            SDL_CloseAudio();
            if (SDL_OpenAudio(&want, NULL) == 0) {
                got = want;
                goto SUCCESS;
            }
            else {
                return;
            }
        }
        else {
            goto SUCCESS;
        }
    }

    return;

SUCCESS:
    enabled = true;
    info.channels = got.channels;
    info.freq = got.freq;
    info.format = cpymo_backend_audio_s16;
    SDL_PauseAudio(0);
}

void cpymo_backend_audio_free(void)
{
    if (enabled)
        SDL_CloseAudio();
}

void cpymo_backend_audio_lock(void)
{
    if (enabled)
        SDL_LockAudio();
}

void cpymo_backend_audio_unlock(void)
{
    if (enabled)
        SDL_UnlockAudio();
}

#endif

#if (defined DISABLE_AUDIO || defined ENABLE_SDL_MIXER_AUDIO_BACKEND)
void cpymo_backend_audio_init(void) {}
void cpymo_backend_audio_free(void) {}
#endif
