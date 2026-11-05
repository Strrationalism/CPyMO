/* NDS interface language; AGPL-3.0-or-later. Game text is never translated. */
#ifndef CPYMO_NDS_UI_LANGUAGE_H
#define CPYMO_NDS_UI_LANGUAGE_H
enum { NDS_LANG_AUTO=0, NDS_LANG_CHINESE=1, NDS_LANG_TRADITIONAL=2, NDS_LANG_ENGLISH=3, NDS_LANG_JAPANESE=4, NDS_LANG_COUNT=5 };
int nds_language_choice_get(void);
int nds_language_choice_set(int);
const char* nds_language_choice_name(int);
int nds_language_effective(void);
void nds_language_boot(void);
void nds_language_load(void);
int nds_language_get(void);
int nds_language_english(void);
int nds_language_set(int);
int nds_language_take_refresh(void);
const char* nds_ui(const char*);
#endif
