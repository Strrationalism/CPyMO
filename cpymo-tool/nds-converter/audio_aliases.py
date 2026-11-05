"""MIT. Exact-content audio deduplication in an owned new conversion directory."""
from pathlib import Path
import hashlib,struct,zlib

def deduplicate_audio(cache,rows,key):
    groups={};records=[];saved=0
    for row in sorted(rows,key=lambda a:(a['kind'],key(a['name']))):
        identity=(row['kind'],row['decoded_pcm_sha256']);suffix='.nar' if row['encoding']=='NAR2' else '.pcm';path=cache/row['kind']/(key(row['name'])+suffix)
        if identity not in groups:groups[identity]=(row,path);continue
        canonical,target=groups[identity]
        # A bounded table is optional. Extra duplicates remain ordinary files.
        if len(records)>=1024:continue
        if path.suffix!=target.suffix or hashlib.sha256(path.read_bytes()).digest()!=hashlib.sha256(target.read_bytes()).digest():raise ValueError('Duplicate audio encoding differs')
        source_key=key(row['name']);target_key=key(canonical['name']);channel={'bgm':'0','se':'1','voice':'2'}[row['kind']]
        records.append((channel+source_key+target_key).encode('ascii'));saved+=row['bytes'];row['alias_original_bytes']=row['bytes'];row['bytes']=0;row['alias_target']=target.relative_to(cache).as_posix();path.unlink()
    if records:
        data=b''.join(sorted(records));(cache/'audio-aliases.bin').write_bytes(struct.pack('<4sII',b'NAD1',len(records),zlib.adler32(data))+data)
    return {'aliases':len(records),'saved_bytes':saved,'index_bytes':12+33*len(records) if records else 0}
