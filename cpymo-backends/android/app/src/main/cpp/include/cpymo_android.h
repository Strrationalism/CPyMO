#ifndef CPYMO_CPYMO_ANDROID_H
#define CPYMO_CPYMO_ANDROID_H

#define SOUND_ENTER 1
#define SOUND_MENU 2
#define SOUND_SELECT 3

void cpymo_android_text_to_speech(const char* text);
void cpymo_android_play_sound(int sound_type);

#endif //CPYMO_CPYMO_ANDROID_H
