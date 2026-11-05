/* CPyMO for NDS 1.2: portable per-game screen-mode preference grammar.
 * No libnds dependency; tested by the host CI with exact serialized bytes.
 * Mode 0: classic top, 1: classic bottom, 2: enhanced dual.
 */
#ifndef CPYMO_NDS_DISPLAY_MODE_PREFS_H
#define CPYMO_NDS_DISPLAY_MODE_PREFS_H
#include <stdio.h>
#include <string.h>

/* Parse exactly one line. CRLF, LF and EOF are accepted; extra tokens,
 * overflow and unsupported future modes default safely to classic.
 * Compatibility: the 1.2 prototype wrote a single '1' for enhanced mode. */
static inline unsigned nds_display_pref_parse(const char *src){
 char text[48];
 if(!src)return 0;
 size_t n=strcspn(src,"\r\n");
 if(n>=sizeof text)return 0;
 memcpy(text,src,n);
 text[n]=0;
 if(!strcmp(text,"1"))return 2;
 unsigned mode=0;
 int consumed=0;
 if(sscanf(text,"NDSDISPLAY2 %u %n",&mode,&consumed)!=1 ||
    consumed<=0 || text[consumed]!=0 || mode>2)return 0;
 return mode;
}
static inline int nds_display_pref_format(char *dst,size_t capacity,unsigned mode){
 if(!dst||capacity<sizeof("NDSDISPLAY2 0\n")||mode>2)return 0;
 int n=snprintf(dst,capacity,"NDSDISPLAY2 %u\n",mode);
 return n>0 && (size_t)n<capacity;
}
#endif
