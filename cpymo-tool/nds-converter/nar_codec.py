"""Experimental bounded lossless PCM16 predictor/Rice blocks, mono 22050Hz."""
import struct,zlib
def wrap(x):return (x+32768)%65536-32768
def encode_block(raw):
 samples=struct.unpack('<'+'h'*(len(raw)//2),raw);best=(len(raw)*8,255,0,raw)
 for order in (0,1,2):
  if len(samples)<=order:continue
  residual=[]
  for i in range(order,len(samples)):
   pred=0 if order==0 else samples[i-1] if order==1 else 2*samples[i-1]-samples[i-2]
   d=wrap(samples[i]-pred);residual.append((d<<1)^(d>>15))
  for k in range(16):
   if max(residual)>>k>31:continue
   bits=sum((x>>k)+1+k for x in residual)+order*16
   if bits>=best[0]:continue
   best=(bits,order,k,residual)
 _,order,k,residual=best
 if order==255:return order,k,raw
 out=bytearray(raw[:order*2]);acc=used=0
 def put(value,n):
  nonlocal acc,used
  acc=(acc<<n)|value;used+=n
  while used>=8:used-=8;out.append((acc>>used)&255)
  acc&=(1<<used)-1
 for x in residual:put(1,(x>>k)+1);put(x&((1<<k)-1),k)
 if used:out.append((acc<<(8-used))&255)
 return order,k,bytes(out)
def decode_block(data,n,order,k):
 if order==255:
  if len(data)!=2*n:raise ValueError('raw size')
  return data
 if order>2 or k>15 or n<=order or len(data)<2*order:raise ValueError('parameters')
 samples=list(struct.unpack('<'+'h'*order,data[:2*order]));bit=order*16
 def get(count):
  nonlocal bit
  if bit+count>len(data)*8:raise ValueError('truncated bits')
  v=0
  for _ in range(count):v=(v<<1)|((data[bit//8]>>(7-bit%8))&1);bit+=1
  return v
 for i in range(order,n):
  q=0
  while not get(1):
   q+=1
   if q>31:raise ValueError('unary bound')
  v=(q<<k)|get(k)
  if v>65535:raise ValueError('residual bound')
  d=(v>>1)^-(v&1);pred=0 if order==0 else samples[-1] if order==1 else 2*samples[-1]-samples[-2]
  samples.append(wrap(pred+d))
 if (bit+7)//8!=len(data) or (bit%8 and data[-1]&((1<<(8-bit%8))-1)):raise ValueError('padding')
 return struct.pack('<'+'h'*n,*samples)
def pack_audio(raw):
 if len(raw)%2:raise ValueError('odd PCM')
 out=bytearray(struct.pack('<4sIII',b'NAR1',len(raw)//2,22050,0))
 for p in range(0,len(raw),2048):
  block=raw[p:p+2048];order,k,data=encode_block(block)
  out.extend(struct.pack('<HHBBHI',len(data),len(block)//2,order,k,0,zlib.adler32(block)));out.extend(data)
 return bytes(out)
def unpack_audio(data):
 magic,n,rate,res=struct.unpack_from('<4sIII',data)
 if magic!=b'NAR1' or rate!=22050 or res:raise ValueError('header')
 p=16;out=bytearray()
 while n:
  size,count,order,k,res,checksum=struct.unpack_from('<HHBBHI',data,p);p+=12
  if count!=min(n,1024) or not size or size>count*2 or res:raise ValueError('block header')
  raw=decode_block(data[p:p+size],count,order,k);p+=size
  if zlib.adler32(raw)!=checksum:raise ValueError('checksum')
  out.extend(raw);n-=count
 if p!=len(data):raise ValueError('trailing')
 return bytes(out)
