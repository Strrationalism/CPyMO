"""MIT. NPS1 exact row-span image storage; no external array dependency."""
import struct

def compress_sparse(span,ratio=.5):
    """NPS2: same row table, independently checked <=8KiB LZ4/raw chunks."""
    import lz4.block,zlib
    if span is None:return None
    rows=int.from_bytes(span[10:12],'little');offset=16+rows*4
    out=bytearray(b'NPS2'+span[4:offset]);payload=memoryview(span)[offset:]
    for pos in range(0,len(payload),8192):
        raw=bytes(payload[pos:pos+8192]);packed=lz4.block.compress(raw,mode='high_compression',compression=12,store_size=False)
        # Avoid spending DS CPU on slightly compressible RGB. Raw chunks have
        # the same length/bounds integrity as NPS1/NPI1; compressed chunks add
        # decoded-data checksums. This container is still private research.
        data=packed if len(packed)<=len(raw)*ratio else raw
        out.extend(struct.pack('<HHI',len(data),len(raw),zlib.adler32(raw) if len(data)<len(raw) else 0));out.extend(data)
    return bytes(out)

def expand_sparse(data):
    import lz4.block,zlib
    magic,w,h,pw,ph,a=struct.unpack_from('<4sHHHHI',data)
    if magic!=b'NPS2' or not w or not h or not pw or not ph or pw>2048 or ph>2048 or pw*ph>262144 or a!=1 or len(data)<16+ph*4:raise ValueError('Invalid compressed sparse header')
    spans=[struct.unpack_from('<HH',data,16+y*4) for y in range(ph)]
    if any(x>pw or n>pw-x for x,n in spans):raise ValueError('Invalid row table')
    left=sum(n for _,n in spans)*3;pos=16+ph*4;out=bytearray(b'NPS1'+data[4:pos])
    while left:
        if len(data)-pos<8:raise ValueError('Truncated chunk header')
        packed,raw,check=struct.unpack_from('<HHI',data,pos);pos+=8
        if raw!=min(left,8192) or not packed or packed>raw or packed>len(data)-pos:raise ValueError('Invalid chunk limits')
        block=data[pos:pos+packed];pos+=packed
        if packed<raw:
            try:block=lz4.block.decompress(block,uncompressed_size=raw)
            except Exception as error:raise ValueError('Invalid LZ4 chunk') from error
        if len(block)!=raw or (zlib.adler32(block)!=check if packed<raw else check!=0):raise ValueError('Chunk checksum')
        out.extend(block);left-=raw
    if pos!=len(data):raise ValueError('Trailing bytes')
    return bytes(out)
def encode_sparse(raw):
    magic,w,h,pw,ph,alpha=struct.unpack_from('<4sHHHHI',raw)
    if magic!=b'NPI1' or alpha!=1 or len(raw)!=16+pw*ph*3:return None
    colors=memoryview(raw)[16:16+pw*ph*2];mask=memoryview(raw)[16+pw*ph*2:]
    spans=[];pixel_rows=[];mask_rows=[]
    for y in range(ph):
        start=y*pw;left=0;right=pw
        while left<right and mask[start+left]==0 and colors[(start+left)*2]==0 and colors[(start+left)*2+1]==128:left+=1
        while right>left and mask[start+right-1]==0 and colors[(start+right-1)*2]==0 and colors[(start+right-1)*2+1]==128:right-=1
        if left==right:left=right=0
        spans.append(struct.pack('<HH',left,right-left));pixel_rows.append(colors[(start+left)*2:(start+right)*2]);mask_rows.append(mask[start+left:start+right])
    return b'NPS1'+raw[4:16]+b''.join(spans+pixel_rows+mask_rows)
def decode_sparse(data):
    if data[:4]==b'NPS2':data=expand_sparse(data)
    magic,w,h,pw,ph,a=struct.unpack_from('<4sHHHHI',data)
    if magic!=b'NPS1' or not w or not h or not pw or not ph or pw>2048 or ph>2048 or pw*ph>262144 or a!=1 or len(data)<16+ph*4:raise ValueError('Invalid sparse header')
    spans=[struct.unpack_from('<HH',data,16+y*4) for y in range(ph)]
    if any(x>pw or n>pw-x for x,n in spans) or len(data)!=16+ph*4+sum(n for _,n in spans)*3:raise ValueError('Invalid sparse layout')
    pixels=bytearray(b'\0\x80'*(pw*ph));mask=bytearray(pw*ph);pos=16+ph*4
    for y,(x,n) in enumerate(spans):pixels[(y*pw+x)*2:(y*pw+x+n)*2]=data[pos:pos+n*2];pos+=n*2
    for y,(x,n) in enumerate(spans):mask[y*pw+x:y*pw+x+n]=data[pos:pos+n];pos+=n
    return b'NPI1'+data[4:16]+pixels+mask
