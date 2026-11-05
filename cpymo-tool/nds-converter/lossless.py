"""MIT. Streaming host lossless conversion, original PCM/RGB555 bytes preserved."""
from pathlib import Path
import ctypes,sys,struct,zlib,hashlib
from nar_codec import encode_block,decode_block
ROOT=Path(getattr(sys,'_MEIPASS',Path(__file__).resolve().parent))
_native=None
if (ROOT/'lossless-native.dll').is_file():
    _native=ctypes.CDLL(str(ROOT/'lossless-native.dll'))
    _native.nar_encode_block.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_void_p,ctypes.c_uint,ctypes.POINTER(ctypes.c_uint),ctypes.POINTER(ctypes.c_uint)]
    _native.nar_encode_block.restype=ctypes.c_uint
    _native.nar_validate_block.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_void_p,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint]
    _native.nar_validate_block.restype=ctypes.c_uint
    _native.pack_encode.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_void_p,ctypes.c_uint,ctypes.POINTER(ctypes.c_uint)]
    _native.pack_encode.restype=ctypes.c_uint
    _native.part_encode.argtypes=_native.pack_encode.argtypes
    _native.part_encode.restype=ctypes.c_uint
    _native.nar2_validate_block.argtypes=_native.nar_validate_block.argtypes
    _native.nar2_validate_block.restype=ctypes.c_uint

def encode_audio(source,destination=None,validate_ds=None):
    """Optional destination; a whole-dataset audit can discard encoded bytes."""
    source=Path(source);size=source.stat().st_size
    if size%2 or size//2>0xffffffff:raise ValueError('Invalid PCM16 sample count')
    header=struct.pack('<4sIII',b'NAR2',size//2,22050,0)
    output=Path(destination).open('xb') if destination is not None else None
    digest=hashlib.sha256(header);original=hashlib.sha256();stored=len(header);blocks=0
    buf=ctypes.create_string_buffer(2048);extra=ctypes.create_string_buffer(2048);order=ctypes.c_uint();key=ctypes.c_uint();alt_order=ctypes.c_uint()
    try:
        if output:output.write(header)
        with source.open('rb') as inp:
            while raw:=inp.read(2048):
                n=len(raw)//2
                if _native:
                    count=_native.nar_encode_block(raw,n,buf,2048,ctypes.byref(order),ctypes.byref(key))
                    if not count or not _native.nar2_validate_block(buf,count,raw,n,order.value,key.value):raise ValueError('Native lossless round trip failed')
                    o,k,data=order.value,key.value,buf.raw[:count]
                    for fn,part in ((_native.pack_encode,5),(_native.part_encode,6)):
                        other=fn(raw,n,extra,part,ctypes.byref(alt_order))
                        if not other:raise ValueError('Native encoder rejected valid PCM')
                        if other<len(data) or (part==5 and other==len(data)):
                            o,k,data=alt_order.value,0 if alt_order.value==255 else part,extra.raw[:other]
                    if not _native.nar2_validate_block(data,len(data),raw,n,o,k):raise ValueError('NAR2 native round trip failed')
                else:
                    o,k,data=encode_block(raw)
                    if decode_block(data,n,o,k)!=raw:raise ValueError('Lossless round trip failed')
                if validate_ds:validate_ds(data,n,o,k,raw)
                packet=struct.pack('<HHBBHI',len(data),n,o,k,0,zlib.adler32(raw))+data
                if output:output.write(packet)
                digest.update(packet);original.update(raw);stored+=len(packet);blocks+=1
    finally:
        if output:output.close()
    return {'pcm_bytes':size,'nar_bytes':stored,'samples':size//2,'blocks':blocks,'pcm_sha256':original.hexdigest(),'nar_sha256':digest.hexdigest(),'validated':True,'encoder':'native' if _native else 'python'}

def image_bytes(raw):
    """NPA4: same bounded 8KiB layout as tested DS decoder; choose HC or raw."""
    import lz4.block
    magic,w,h,pw,ph,alpha=struct.unpack_from('<4sHHHHI',raw)
    if magic!=b'NPI1' or alpha>1 or len(raw)!=16+pw*ph*(3 if alpha else 2):raise ValueError('Invalid NPI1')
    out=bytearray(b'NPA4'+raw[4:16]);p=16
    for size in ([pw*ph*2,pw*ph] if alpha else [pw*ph*2]):
        end=p+size
        while p<end:
            block=raw[p:min(p+8192,end)];packed=lz4.block.compress(block,mode='high_compression',compression=12,store_size=False)
            if lz4.block.decompress(packed,uncompressed_size=len(block))!=block:raise ValueError('Image round trip failed')
            data=packed if len(packed)<len(block) else block
            out.extend(struct.pack('<HHI',len(data),len(block),zlib.adler32(block)));out.extend(data);p+=len(block)
    return bytes(out)


def image_candidate(raw,compress_spans=False):
    from image_sparse import encode_sparse,compress_sparse
    packed=image_bytes(raw)
    chosen=packed if len(packed)<=len(raw)*.2 else raw
    span=encode_sparse(raw)
    if span is not None and len(span)<len(chosen) and len(span)<=len(raw)*.9:chosen=span
    compressed_span=compress_sparse(span) if compress_spans else None
    if compressed_span is not None and len(compressed_span)<len(chosen) and len(compressed_span)<=len(raw)*.9:chosen=compressed_span
    return chosen
