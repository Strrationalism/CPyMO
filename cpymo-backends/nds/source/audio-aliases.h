/* Exact-content aliases, loaded once per game; no lookup I/O during playback.
 * NAD1: magic, LE count, Adler32, sorted 33-byte records (channel,from,to).
 * Hashes are lowercase ASCII hex; targets must not be aliases. */
volatile unsigned alias_state[8]={0x4e414431};
static uint32_t alias_checksum(const unsigned char* p,unsigned n){
 uint32_t a=1,b=0;while(n){unsigned k=n>4096?4096:n;n-=k;while(k--){a+=*p++;b+=a;}a%=65521;b%=65521;}return (b<<16)|a;
}
static int alias_find(const Audio* a,const unsigned char key[17]){
 unsigned lo=0,hi=a->alias_count;while(lo<hi){unsigned mid=(lo+hi)/2;int d=memcmp(key,a->aliases+mid*33,17);if(!d)return mid;if(d<0)hi=mid;else lo=mid+1;}return -1;
}
static error_t load_aliases(Audio* a,const char* dir){
 if(a->alias_checked)return a->alias_error;
 a->alias_checked=true;alias_state[1]++;char path[512];snprintf(path,sizeof path,"%s/nds-cache/audio-aliases.bin",dir);FILE* f=fopen(path,"rb");
 if(!f){if(errno==ENOENT)return CPYMO_ERR_SUCC;goto bad;}
 unsigned char h[12];if(fread(h,1,12,f)!=12||memcmp(h,"NAD1",4))goto close_bad;
 unsigned n=h[4]|((unsigned)h[5]<<8)|((unsigned)h[6]<<16)|((unsigned)h[7]<<24);uint32_t check=h[8]|((uint32_t)h[9]<<8)|((uint32_t)h[10]<<16)|((uint32_t)h[11]<<24);
 if(!n||n>1024)goto close_bad;
 a->aliases=malloc(n*33);if(!a->aliases){fclose(f);a->alias_error=CPYMO_ERR_OUT_OF_MEM;return a->alias_error;}
 if(fread(a->aliases,33,n,f)!=n||fgetc(f)!=EOF||ferror(f)||alias_checksum(a->aliases,n*33)!=check)goto close_bad;
 a->alias_count=n;
 for(unsigned i=0;i<n;i++){
  unsigned char* row=a->aliases+i*33;if(row[0]<'0'||row[0]>'2'||!memcmp(row+1,row+17,16)||(i&&memcmp(row-33,row,17)>=0))goto close_bad;
  for(unsigned j=1;j<33;j++)if(!((row[j]>='0'&&row[j]<='9')||(row[j]>='a'&&row[j]<='f')))goto close_bad;
  unsigned char target[17];target[0]=row[0];memcpy(target+1,row+17,16);if(alias_find(a,target)>=0)goto close_bad;
 }
 fclose(f);alias_state[2]+=n;alias_state[5]=n*33;return CPYMO_ERR_SUCC;
close_bad:fclose(f);free(a->aliases);a->aliases=NULL;a->alias_count=0;
bad:alias_state[4]++;engine_state[19]++;a->alias_error=CPYMO_ERR_BAD_FILE_FORMAT;return a->alias_error;
}
static void audio_cache_path(char* out,unsigned cap,const char* dir,const char* folder,cpymo_str name,const char* ext,Audio* a,unsigned id){
 nds_cache_path(out,cap,dir,folder,name,ext);if(!a->alias_count)return;
 char* tail=strrchr(out,'/');if(!tail||strlen(++tail)<16)return;
 unsigned char key[17];key[0]='0'+id;memcpy(key+1,tail,16);int index=alias_find(a,key);
 if(index>=0){char canonical[17];memcpy(canonical,a->aliases+index*33+17,16);canonical[16]=0;nds_cache_hash_path(out,cap,dir,folder,canonical,ext);alias_state[3]++;}else alias_state[6]++;
}
