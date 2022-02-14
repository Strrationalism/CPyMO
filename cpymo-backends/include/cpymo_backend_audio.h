#ifndef INCLUDE_CPYMO_BACKEND_AUDIO
#define INCLUDE_CPYMO_BACKEND_AUDIO

#include <stdbool.h>

typedef enum {
	cpymo_backend_audio_s16le
} cpymo_backend_audio_format;

typedef struct {
	size_t freq;
	cpymo_backend_audio_format format;
	size_t channels;
} cpymo_backend_audio_info;

// Returns NULL to disable audio.
const cpymo_backend_audio_info *cpymo_backend_audio_get_info(void);

void cpymo_backend_audio_lock(void);
void cpymo_backend_audio_unlock(void);

#endif
