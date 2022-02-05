#include <3ds.h>
#include <string.h>
#include <stdio.h>

volatile static bool audio_enabled = false;

#define SAMPLERATE 44100
#define SAMPLESPERBUF (SAMPLERATE / 60)
#define BYTEPERSAMPLE 2

static u8 *audio_buf;

static ndspWaveBuf waveBuf[2];

static void cpymo_backend_audio_callback(void *_) 
{
    static unsigned double_buffering = 0;
    s8 *dest = waveBuf[double_buffering].data_pcm8;

	for (int i = 0; i < waveBuf[double_buffering].nsamples; i++) {
		dest[i * 2 + 1] = 0;
        dest[i * 2] = 0;
	}

	DSP_FlushDataCache(dest, waveBuf[double_buffering].nsamples * BYTEPERSAMPLE);
    ndspChnWaveBufAdd(0, &waveBuf[double_buffering]);
    double_buffering = !double_buffering;
}

void cpymo_backend_audio_init() 
{
    if(R_FAILED(ndspInit())) {
        printf("[Error] Failed to initialize audio.\n");
        return;
    }

    size_t buf_size = SAMPLESPERBUF * BYTEPERSAMPLE * 2;

    audio_buf = (u8*)linearAlloc(buf_size);
    if(audio_buf == NULL) {
        ndspExit();
        printf("[Error] Failed to alloc audio buf.\n");
        return;
    }

    memset(audio_buf, 0, buf_size);

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, 44100);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM8);
    
    float mix[12];
    memset(mix, 0, sizeof(mix));
    mix[0] = 1.0f;
    mix[1] = 1.0f;
    ndspChnSetMix(0, mix);

    memset(waveBuf, 0, sizeof(waveBuf));
    waveBuf[0].data_vaddr = audio_buf;
    waveBuf[0].nsamples = SAMPLESPERBUF;
    waveBuf[1].data_vaddr = audio_buf + SAMPLESPERBUF * BYTEPERSAMPLE;
    waveBuf[1].nsamples = SAMPLESPERBUF;

    ndspChnWaveBufAdd(0, &waveBuf[0]);
    ndspChnWaveBufAdd(0, &waveBuf[1]);

    ndspSetCallback(cpymo_backend_audio_callback, NULL);

    audio_enabled = true;
}

void cpymo_backend_audio_free() {
    if(audio_enabled) {
        audio_enabled = false;

        ndspExit();
        linearFree(audio_buf);
    }
}
