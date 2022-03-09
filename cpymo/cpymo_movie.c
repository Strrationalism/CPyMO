#include "cpymo_movie.h"
#include "cpymo_engine.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

typedef struct {
	AVFormatContext *format_context;
	int video_stream_index, audio_stream_index;

	AVCodecContext *video_codec_context, *audio_codec_context;
} cpymo_movie;

static error_t cpymo_movie_update(cpymo_engine *e, void *ui_data, float dt)
{
	cpymo_ui_exit(e);
	return CPYMO_ERR_SUCC;
}

static void cpymo_movie_draw(const cpymo_engine *e, const void *ui_data)
{

}

static void cpymo_movie_delete(cpymo_engine *e, void *ui_data)
{
	cpymo_movie *m = (cpymo_movie *)ui_data;

	if (m->audio_codec_context) avcodec_free_context(&m->audio_codec_context);
	if (m->video_codec_context) avcodec_free_context(&m->video_codec_context);
	if (m->format_context) avformat_close_input(&m->format_context);
}

error_t cpymo_movie_play(cpymo_engine * e, const char *path)
{
	cpymo_movie *m = NULL;
	error_t err = 
		cpymo_ui_enter(
			(void **)&m, 
			e, 
			sizeof(*m), 
			&cpymo_movie_update, 
			&cpymo_movie_draw, 
			&cpymo_movie_delete);
	CPYMO_THROW(err);

	m->format_context = NULL;
	m->video_codec_context = NULL;
	m->audio_codec_context = NULL;

	#define THROW(ERR_COND, ERRCODE, MESSAGE) \
		if (ERR_COND) { \
			printf("[Error] %s: %s.\n", MESSAGE, path); \
			cpymo_ui_exit(e); \
			return ERRCODE; \
		}

	#define THROW_AVERR(ERR_COND, ERRCODE) \
		THROW(ERR_COND, ERRCODE, av_err2str(averr));

	int averr = avformat_open_input(&m->format_context, path, NULL, NULL);
	THROW_AVERR(averr, CPYMO_ERR_CAN_NOT_OPEN_FILE);

	averr = avformat_find_stream_info(m->format_context, NULL);
	THROW_AVERR(averr < 0, CPYMO_ERR_NOT_FOUND);

	AVCodec *video_codec = NULL;
	m->video_stream_index = av_find_best_stream(m->format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
	THROW(m->video_stream_index < 0, CPYMO_ERR_NOT_FOUND, "Can not find video stream");
	THROW(video_codec == NULL, CPYMO_ERR_NOT_FOUND, "Can not find video codec");

	m->video_codec_context = avcodec_alloc_context3(video_codec);
	THROW(m->video_codec_context == NULL, CPYMO_ERR_UNKNOWN, "Can not open video codec context");

	averr = avcodec_open2(m->video_codec_context, video_codec, NULL);
	THROW_AVERR(averr, CPYMO_ERR_UNKNOWN);

	averr = avcodec_parameters_to_context(
		m->video_codec_context, 
		m->format_context->streams[m->video_stream_index]->codecpar);
	THROW_AVERR(averr < 0, CPYMO_ERR_UNKNOWN);

	AVCodec *audio_codec = NULL;
	m->audio_stream_index = av_find_best_stream(m->format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_codec, 0);
	if (m->audio_stream_index >= 0 && audio_codec && e->audio.enabled) {
		m->audio_codec_context = avcodec_alloc_context3(audio_codec);
		if (m->audio_codec_context == NULL) goto AUDIO_FAILED;

		if (avcodec_open2(m->audio_codec_context, audio_codec, NULL)) {
			avcodec_free_context(&m->audio_codec_context);
			goto AUDIO_FAILED;
		}

		if (avcodec_parameters_to_context(
			m->audio_codec_context,
			m->format_context->streams[m->audio_stream_index]->codecpar)) {
			avcodec_free_context(&m->audio_codec_context);
			goto AUDIO_FAILED;
		}
	}
	else {
		AUDIO_FAILED:
		audio_codec = NULL;
		m->audio_codec_context = NULL;
	}

	return CPYMO_ERR_SUCC;
}
