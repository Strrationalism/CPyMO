#include <SDL.h>
#include <stdbool.h>
#include <cpymo_error.h>

static bool audio_enabled;

static void cpymo_backend_audio_sdl_callback(void *userdata, Uint8 * stream, int len)
{
	SDL_memset(stream, 0, len);
}

bool cpymo_backend_audio_enabled()
{
	return audio_enabled;
}

void cpymo_backend_audio_init()
{
	SDL_AudioSpec want;
	
	SDL_memset(&want, 0, sizeof(want));
	want.callback = &cpymo_backend_audio_sdl_callback;
	want.channels = 2;
	want.format = AUDIO_S16;
	want.freq = 44100;
	want.samples = 32768;
	
	if (SDL_OpenAudio(&want, NULL) == 0) {
		SDL_PauseAudio(0);
		audio_enabled = true;
	}
	else {
		audio_enabled = false;
		const char *err = SDL_GetError();
		SDL_Log("[Error] Audio device open failed: %s.", err);
	}
}

void cpymo_backend_audio_free()
{
	if (audio_enabled) {
		SDL_CloseAudio();
	}
}
