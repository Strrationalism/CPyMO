#ifndef INCLUDE_CPYMO_AUDIO
#define INCLUDE_CPYMO_AUDIO

#include <stddef.h>
#include <stdbool.h>

#include "cpymo_package.h"
#include "cpymo_error.h"

#define CPYMO_AUDIO_MAX_CHANNELS 3
#define CPYMO_AUDIO_CHANNEL_BGM 0
#define CPYMO_AUDIO_CHANNEL_SE 1
#define CPYMO_AUDIO_CHANNEL_VO 2

#ifdef DISABLE_AUDIO
#ifndef DISABLE_FFMPEG_AUDIO
#define DISABLE_FFMPEG_AUDIO
#endif
#endif

#ifdef DISABLE_AUDIO
typedef struct {
	float volumes[CPYMO_AUDIO_MAX_CHANNELS];
} cpymo_audio_system;
#endif

#if (!defined DISABLE_FFMPEG_AUDIO)

#ifdef __CXX
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#ifdef __CXX
}
#endif

typedef struct {
	bool enabled, loop;

	float volume;
	
	AVFormatContext *format_context;
	AVCodecContext *codec_context;
	SwrContext *swr_context;

	AVPacket *packet;
	AVFrame *frame;
	uint8_t *converted_buf;
	size_t converted_buf_size, converted_buf_all_size;

	size_t converted_frame_current_offset;

	AVIOContext *io_context;
	cpymo_package_stream_reader package_reader;

	int stream_id;
} cpymo_audio_channel;

typedef struct {
	bool enabled;
	cpymo_audio_channel channels[CPYMO_AUDIO_MAX_CHANNELS];

	char *bgm_name, *se_name;
} cpymo_audio_system;

#elif (!defined DISABLE_AUDIO)
typedef void *cpymo_audio_system;
#endif

void cpymo_audio_copy_mixed_samples(void * dst, size_t len, cpymo_audio_system *s);
bool cpymo_audio_channel_get_samples(
	void **samples,
	size_t *in_out_len,
	size_t channelID,
	cpymo_audio_system *);

void cpymo_audio_init(cpymo_audio_system *);
void cpymo_audio_free(cpymo_audio_system *);

float cpymo_audio_get_channel_volume(size_t cid, const cpymo_audio_system *s);

void cpymo_audio_set_channel_volume(size_t cid, cpymo_audio_system *s, float vol);

bool cpymo_audio_channel_is_playing(size_t cid, cpymo_audio_system *s);
bool cpymo_audio_channel_is_looping(size_t cid, cpymo_audio_system *s);

struct cpymo_engine;
bool cpymo_audio_enabled(struct cpymo_engine *e);

bool cpymo_audio_wait_se(struct cpymo_engine *, float);

error_t cpymo_audio_bgm_play(struct cpymo_engine *e, cpymo_str bgmname, bool loop);
void cpymo_audio_bgm_stop(struct cpymo_engine *e);

error_t cpymo_audio_se_play(struct cpymo_engine *e, cpymo_str sename, bool loop);
void cpymo_audio_se_stop(struct cpymo_engine *e);

error_t cpymo_audio_vo_play(struct cpymo_engine *e, cpymo_str voname);
void cpymo_audio_vo_stop(struct cpymo_engine *e);

error_t cpymo_audio_play_video(struct cpymo_engine *e, const char *path);

const char *cpymo_audio_get_bgm_name(struct cpymo_engine *e);
const char *cpymo_audio_get_se_name(struct cpymo_engine *e);

#endif
