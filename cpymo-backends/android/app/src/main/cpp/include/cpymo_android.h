#ifndef CPYMO_CPYMO_ANDROID_H
#define CPYMO_CPYMO_ANDROID_H

#define SOUND_ENTER 1
#define SOUND_LOAD 2
#define SOUND_MENU 3
#define SOUND_MOUSE_DOWN 4
#define SOUND_MOUSE_UP 5
#define SOUND_SAVE 6
#define SOUND_SELECT 7

int cpymo_android_text_to_speech(const char* text);
void cpymo_android_play_sound(int sound_type);

#endif //CPYMO_CPYMO_ANDROID_H
