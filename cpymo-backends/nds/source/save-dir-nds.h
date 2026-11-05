/* Per-game NDS FAT save path policy, host-testable and independent of libnds.
 * Existing saves are never deleted or replaced when their directory exists. */
#ifndef CPYMO_NDS_SAVE_DIR_H
#define CPYMO_NDS_SAVE_DIR_H
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static inline int nds_save_ensure_directory(const char* gamedir){
 if(!gamedir || !*gamedir)return 0;
 char dir[512];
 int count=snprintf(dir,sizeof(dir),"%s/save",gamedir);
 if(count<0 || (size_t)count>=sizeof(dir))return 0;
 if(mkdir(dir,0777)==0)return 1;
 if(errno!=EEXIST)return 0;
 struct stat st;
 return stat(dir,&st)==0 && S_ISDIR(st.st_mode);
}

static inline FILE* nds_save_fopen(const char* gamedir,const char* name,const char* mode){
 if(!gamedir||!name||!mode||!name[0]||!mode[0])return NULL;
 if((strchr(mode,'w')||strchr(mode,'a')) && !nds_save_ensure_directory(gamedir))
  return NULL;
 char path[512];
 int count=snprintf(path,sizeof(path),"%s/save/%s",gamedir,name);
 if(count<0 || (size_t)count>=sizeof(path))return NULL;
 return fopen(path,mode);
}
#endif
