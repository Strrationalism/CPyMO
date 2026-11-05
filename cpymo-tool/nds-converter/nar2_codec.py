"""MIT. Independent Python reference decoder for bounded NAR1/NAR2 audio."""
import struct,zlib
from nar_codec import decode_block as decode_legacy,wrap
def decode_block(data,n,order,k):
    if not 0<n<=1024 or not 0<len(data)<=n*2:raise ValueError('Block limits')
    if order<=2 or order==255:
        if order==255 and k:raise ValueError('Raw parameters')
        return decode_legacy(data,n,order,k)
    if order in (16,17,18):po=order-16;partition=False
    elif order in (32,33,34):po=order-32;partition=True
    else:raise ValueError('Unknown block mode')
    if not 5<=k<=7 or n<=po or len(data)<po*2:raise ValueError('Parameters')
    values=list(struct.unpack('<'+'h'*po,data[:po*2]));bit=po*16
    def get(count):
        nonlocal bit
        if bit+count>len(data)*8:raise ValueError('Truncated block')
        v=0
        for _ in range(count):v=(v<<1)|((data[bit//8]>>(7-bit%8))&1);bit+=1
        return v
    while len(values)<n:
        width=get(4 if partition else 5)
        if width>16:raise ValueError('Width')
        end=min(n,len(values)+(1<<k))
        while len(values)<end:
            if partition:
                q=0
                while not get(1):
                    q+=1
                    if q>31:raise ValueError('Unary limit')
                z=(q<<width)|get(width)
                if z>65535:raise ValueError('Residual limit')
            else:z=get(width)
            delta=(z>>1)^-(z&1);pred=0 if po==0 else values[-1] if po==1 else 2*values[-1]-values[-2];values.append(wrap(pred+delta))
    if (bit+7)//8!=len(data) or (bit%8 and data[-1]&((1<<(8-bit%8))-1)):raise ValueError('Padding')
    return struct.pack('<'+'h'*n,*values)
def unpack_audio(data):
    magic,n,rate,res=struct.unpack_from('<4sIII',data)
    if magic not in (b'NAR1',b'NAR2') or rate!=22050 or res:raise ValueError('Header')
    pos=16;out=bytearray()
    while n:
        size,count,order,k,res,check=struct.unpack_from('<HHBBHI',data,pos);pos+=12
        if count!=min(n,1024) or not size or size>count*2 or res or pos+size>len(data):raise ValueError('Block header')
        if magic==b'NAR1' and order>=16 and order!=255:raise ValueError('NAR2 mode in old header')
        raw=decode_block(data[pos:pos+size],count,order,k);pos+=size
        if zlib.adler32(raw)!=check:raise ValueError('Checksum')
        out.extend(raw);n-=count
    if pos!=len(data):raise ValueError('Trailing data')
    return bytes(out)
