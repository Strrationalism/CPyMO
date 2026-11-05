/* CPyMO NDS 1.2.1 test3: low-overhead, integer-only master loudness guard.
 *
 * A restrained +1.50 dB target is applied only while the mixed signal has
 * enough headroom. Peaks are inspected before writing each 64-sample block;
 * gain attacks immediately to prevent digital clipping and recovers over
 * roughly one second, rather than abruptly amplifying a loud transient.
 *
 * No float, heap allocation, sound-hardware overdrive, or game settings.
 * This is intentionally compatible with both MP3 and cached PCM paths.
 */
#ifndef CPYMO_NDS_AUDIO_GAIN_H
#define CPYMO_NDS_AUDIO_GAIN_H
#include <stdint.h>
#define NDS_AUDIO_GAIN_Q12_ONE 4096
#define NDS_AUDIO_GAIN_Q12_TARGET 4868 /* 1.50 dB */
#define NDS_AUDIO_GAIN_PCM_CEILING 32760
#define NDS_AUDIO_GAIN_RELEASE_PER_64 8
typedef struct {
 int gain_q12;
 uint32_t samples;
 uint32_t limited_blocks;
 uint32_t preexisting_over_blocks;
 uint32_t max_input_peak;
} NdsAudioGain;
static inline void nds_audio_gain_reset(NdsAudioGain* s){
 *s=(NdsAudioGain){NDS_AUDIO_GAIN_Q12_TARGET,0,0,0,0};
}
static inline void nds_audio_gain_write(
 int16_t* dest,const int32_t* mixed,unsigned count,NdsAudioGain* state){
 if(!count)return;
 /* All input samples come from at most three signed 16-bit sources:
    maximum 98304, safe for the signed 32-bit fixed-point calculation. */
 int32_t peak=0;
 for(unsigned i=0;i<count;i++){
  int32_t v=mixed[i],m=v<0?-v:v;
  if(m>peak)peak=m;
 }
 if((uint32_t)peak>state->max_input_peak)state->max_input_peak=(uint32_t)peak;
 if(peak>32767)state->preexisting_over_blocks++;
 int target=NDS_AUDIO_GAIN_Q12_TARGET;
 if(peak && peak*NDS_AUDIO_GAIN_Q12_TARGET>
             NDS_AUDIO_GAIN_PCM_CEILING*NDS_AUDIO_GAIN_Q12_ONE){
  target=(NDS_AUDIO_GAIN_PCM_CEILING*NDS_AUDIO_GAIN_Q12_ONE)/peak;
  state->limited_blocks++;
 }
 if(target<state->gain_q12)state->gain_q12=target; /* instant attack */
 else if(target>state->gain_q12){
  int step=(int)((count*NDS_AUDIO_GAIN_RELEASE_PER_64+63)/64);
  if(step<1)step=1;
  if(step>target-state->gain_q12)step=target-state->gain_q12;
  state->gain_q12+=step; /* slow release, never above the safe target */
 }
 for(unsigned i=0;i<count;i++){
  int v=(mixed[i]*state->gain_q12)/NDS_AUDIO_GAIN_Q12_ONE;
  if(v>NDS_AUDIO_GAIN_PCM_CEILING)v=NDS_AUDIO_GAIN_PCM_CEILING;
  if(v< -NDS_AUDIO_GAIN_PCM_CEILING)v=-NDS_AUDIO_GAIN_PCM_CEILING;
  dest[i]=(int16_t)v;
 }
 state->samples+=count;
}
#endif
