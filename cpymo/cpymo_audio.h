#ifndef INCLUDE_CPYMO_AUDIO
#define INCLUDE_CPYMO_AUDIO

#include <stddef.h>

#define CPYMO_AUDIO_MAX_CHANNELS 3
#define CPYMO_AUDIO_CHANNEL_BGM 0
#define CPYMO_AUDIO_CHANNEL_SE 1
#define CPYMO_AUDIO_CHANNEL_VO 2

typedef struct {
	bool enabled;
	void *audio_buf;
	size_t audio_buf_len;
} cpymo_audio_channel;

#endif
