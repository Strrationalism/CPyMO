#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static bool audio_enabled = false;

#define SAMPLERATE 44100
#define SAMPLESPERBUF (SAMPLERATE / 60)
#define BYTEPERSAMPLE 4
#define DSP_FIRM "sdmc:/3ds/dspfirm.cdc"

static s16 *audio_buf;

static ndspWaveBuf waveBuf[2];

bool cpymo_backend_audio_enabled() 
{
    return audio_enabled;
}

static bool cpymo_backend_audio_need_dump_dsp() 
{
    FILE *f = fopen(DSP_FIRM, "rb");
    if(f != NULL) {
        fclose(f);
        return false;
    }
    return true;
}

static void cpymo_backend_audio_callback(void *_) 
{
    static unsigned double_buffering = 0;
    s16 *dest = waveBuf[double_buffering].data_pcm16;

	for (int i = 0; i < waveBuf[double_buffering].nsamples; i++) {
		dest[i * 2 + 1] = rand();
        dest[i * 2] = rand();
	}

	DSP_FlushDataCache(dest, waveBuf[double_buffering].nsamples * BYTEPERSAMPLE);
    ndspChnWaveBufAdd(0, &waveBuf[double_buffering]);
    double_buffering = !double_buffering;
}

void cpymo_backend_audio_init() 
{
    if(cpymo_backend_audio_need_dump_dsp())
        printf("[Error] DSP Firmware not found, you can dump it using DSP1 (https://github.com/zoogie/DSP1).\n");

    if(R_FAILED(ndspInit())) {
        printf("[Error] Failed to initialize audio.\n");
        return;
    }

    size_t buf_size = SAMPLESPERBUF * BYTEPERSAMPLE * 2;

    audio_buf = (s16 *)linearAlloc(buf_size);
    if(audio_buf == NULL) {
        ndspExit();
        printf("[Error] Failed to alloc audio buf.\n");
        return;
    }

    memset(audio_buf, 0, buf_size);

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, 44100);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    
    float mix[12];
    memset(mix, 0, sizeof(mix));
    mix[0] = 1.0f;
    mix[1] = 1.0f;
    ndspChnSetMix(0, mix);

    memset(waveBuf, 0, sizeof(waveBuf));
    waveBuf[0].data_vaddr = audio_buf;
    waveBuf[0].nsamples = SAMPLESPERBUF;
    waveBuf[1].data_vaddr = audio_buf + SAMPLESPERBUF * 2;
    waveBuf[1].nsamples = SAMPLESPERBUF;

    ndspChnWaveBufAdd(0, &waveBuf[0]);
    ndspChnWaveBufAdd(0, &waveBuf[1]);

    ndspSetCallback(cpymo_backend_audio_callback, NULL);

    audio_enabled = true;
}

void cpymo_backend_audio_free() {
    if(audio_enabled) {
        ndspExit();
        linearFree(audio_buf);
    }
}
