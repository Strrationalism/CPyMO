"""Experimental independent LZ4 blocks around unchanged NPI1 image planes.
No whole-game output. Written from the public LZ4 block specification.
"""
import struct,zlib
BLOCK=8192
def length(out,n):
 while n>=255:out.append(255);n-=255
 out.append(n)
def compress(raw):
 n=len(raw);out=bytearray();table={};anchor=i=0
 while i<=n-12:
  k=raw[i:i+4];old=table.get(k);table[k]=i
  if old is None or i-old>65535:i+=1;continue
  end=i+4
  while end<n-5 and raw[old+end-i]==raw[end]:end+=1
  literals=i-anchor;match=end-i-4
  out.append((min(15,literals)<<4)|min(15,match))
  if literals>=15:length(out,literals-15)
  out.extend(raw[anchor:i]);out.extend(struct.pack('<H',i-old))
  if match>=15:length(out,match-15)
  i=end;anchor=i
  for j in range(max(0,i-2),i):table[raw[j:j+4]]=j
 literals=n-anchor;out.append(min(15,literals)<<4)
 if literals>=15:length(out,literals-15)
 out.extend(raw[anchor:]);return bytes(out)
def decompress(data,expected):
 out=bytearray();p=0
 def ext(v):
  nonlocal p
  if v==15:
   while True:
    if p>=len(data):raise ValueError('Truncated length')
    b=data[p];p+=1;v+=b
    if v>expected:raise ValueError('Length exceeds output')
    if b!=255:break
  return v
 while p<len(data):
  tok=data[p];p+=1;lit=ext(tok>>4)
  if lit>len(data)-p or lit>expected-len(out):raise ValueError('Literal bounds')
  out.extend(data[p:p+lit]);p+=lit
  if p==len(data):break
  if p+2>len(data):raise ValueError('Truncated offset')
  off=int.from_bytes(data[p:p+2],'little');p+=2
  if not off or off>len(out):raise ValueError('Invalid offset')
  match=ext(tok&15)+4
  if match>expected-len(out):raise ValueError('Match bounds')
  for _ in range(match):out.append(out[-off])
 if len(out)!=expected:raise ValueError('Output size')
 return bytes(out)
def pack_image(data,adler=False):
 magic,w,h,pw,ph,alpha=struct.unpack_from('<4sHHHHI',data)
 if magic!=b'NPI1' or not pw or not ph or alpha>1 or len(data)!=16+pw*ph*(3 if alpha else 2):raise ValueError('Bad NPI1')
 out=bytearray((b'NPA4' if adler else b'NPL4')+data[4:16]);pos=16
 for size in ([pw*ph*2,pw*ph] if alpha else [pw*ph*2]):
  end=pos+size
  while pos<end:
   raw=data[pos:min(pos+BLOCK,end)];compressed=compress(raw)
   stored=compressed if len(compressed)<len(raw) else raw
   out.extend(struct.pack('<HHI',len(stored),len(raw),(zlib.adler32 if adler else zlib.crc32)(raw)));out.extend(stored);pos+=len(raw)
 return bytes(out)
def unpack_image(data):
 magic,w,h,pw,ph,alpha=struct.unpack_from('<4sHHHHI',data)
 if magic not in (b'NPL4',b'NPA4') or not pw or not ph or pw*ph>262144 or alpha>1:raise ValueError('Bad NPL4')
 out=bytearray(b'NPI1'+data[4:16]);p=16
 for size in ([pw*ph*2,pw*ph] if alpha else [pw*ph*2]):
  done=0
  while done<size:
   if p+8>len(data):raise ValueError('Truncated block header')
   stored,n,crc=struct.unpack_from('<HHI',data,p);p+=8
   if n!=min(BLOCK,size-done) or not stored or stored>n or p+stored>len(data):raise ValueError('Bad block lengths')
   raw=data[p:p+stored] if stored==n else decompress(data[p:p+stored],n);p+=stored
   if (zlib.adler32 if magic==b'NPA4' else zlib.crc32)(raw)!=crc:raise ValueError('Checksum mismatch')
   out.extend(raw);done+=n
 if p!=len(data):raise ValueError('Trailing bytes')
 return bytes(out)
