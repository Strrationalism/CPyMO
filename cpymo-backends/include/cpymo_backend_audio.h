#ifndef INCLUDE_CPYMO_BACKEND_AUDIO
#define INCLUDE_CPYMO_BACKEND_AUDIO

#include <stdbool.h>

// The data described by the sample format is always in native-endian order. 
typedef enum {
	cpymo_backend_audio_s16,
	cpymo_backend_audio_s32,
	cpymo_backend_audio_f32
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

#if !defined NON_VISUALLY_IMPAIRED_HELP && defined __ANDROID__
#define SOUND_ENTER 1
#define SOUND_LOAD 2
#define SOUND_MENU 3
#define SOUND_MOUSE_DOWN 4
#define SOUND_MOUSE_UP 5
#define SOUND_SAVE 6
#define SOUND_SELECT 7

void cpymo_backend_audio_android_play_sound(int sound_type);
#endif

#endif
