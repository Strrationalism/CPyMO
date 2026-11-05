"""Patch UTF-16LE titles in a finished NDS/DSi ROM banner, not its icon.

devkitPro ndstool's -b text routine copies 8-bit input bytes into UTF-16
slots rather than decoding UTF-8; passing a Chinese credit directly would
produce corrupt characters. Patch the six version-1 banner language titles
after packing the ROM, then recompute the NDS CRC16 (IBM/ARC, init FFFF,
no final inversion) across the banner bytes 0x20..0x83F.

The ROM header, icon/palette, both executable cores and all other data stay
byte-identical. This is intentionally separate from the internal game version.
"""
from pathlib import Path
import struct

BANNER_BYTES=0x840
TITLE_START=0x240
TITLE_SLOT_BYTES=0x100
LANGUAGE_COUNT=6


def crc16_nds(blob):
    crc=0xffff
    for byte in blob:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xA001 if crc & 1 else 0)
    return crc


def _load_banner(file):
    file.seek(0,2)
    length=file.tell()
    if length<0x200:
        raise ValueError("NDS ROM missing full header")
    file.seek(0x68)
    location=struct.unpack("<I",file.read(4))[0]
    if location<0x200 or location+BANNER_BYTES>length:
        raise ValueError("NDS banner offset exceeds ROM boundaries")
    file.seek(location)
    banner=bytearray(file.read(BANNER_BYTES))
    if len(banner)!=BANNER_BYTES or struct.unpack_from("<H",banner,0)[0]!=1:
        raise ValueError("Only version-1 NDS banners are supported")
    if struct.unpack_from("<H",banner,2)[0]!=crc16_nds(banner[0x20:]):
        raise ValueError("Existing NDS banner CRC does not match")
    return location,banner


def _encode_titles(lines):
    if len(lines)!=3 or any(not isinstance(line,str) or not line for line in lines):
        raise ValueError("Exactly three nonempty title/edition/credit lines required")
    if any(any(ch in line for ch in "\r\n;") for line in lines):
        raise ValueError("Unexpected delimiter in a single title line")
    encoded="\n".join(lines).encode("utf-16le")
    if len(encoded)>TITLE_SLOT_BYTES-2:
        raise ValueError("Combined banner lines exceed 127 UTF-16 characters")
    return encoded+b"\x00"*(TITLE_SLOT_BYTES-len(encoded))


def read_nds_banner_titles(path):
    with Path(path).open("rb") as f:
        _,banner=_load_banner(f)
        out=[]
        for i in range(LANGUAGE_COUNT):
            start=TITLE_START+i*TITLE_SLOT_BYTES
            slot=banner[start:start+TITLE_SLOT_BYTES].decode("utf-16le")
            out.append(slot.split("\x00",1)[0].split("\n"))
        return out


def patch_nds_banner_titles(path,lines):
    new_title=_encode_titles(lines)
    with Path(path).open("r+b") as f:
        location,banner=_load_banner(f)
        for i in range(LANGUAGE_COUNT):
            start=TITLE_START+i*TITLE_SLOT_BYTES
            banner[start:start+TITLE_SLOT_BYTES]=new_title
        struct.pack_into("<H",banner,2,crc16_nds(banner[0x20:]))
        f.seek(location)
        if f.write(banner)!=BANNER_BYTES:
            raise IOError("Unable to write the full NDS banner")
    actual=read_nds_banner_titles(path)
    if any(slot!=list(lines) for slot in actual):
        raise ValueError("NDS banner UTF-16 verification failed")
