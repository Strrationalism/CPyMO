#!/usr/bin/env python3
"""Independent FVDS container inspector for CPyMO NDS 1.2 research.

Container structure only: does not decode video/audio or establish hardware
compatibility. No third-party player or encoder code is incorporated.
"""
import argparse
import hashlib
import json
import os
import struct
from pathlib import Path

HEADER = struct.Struct("<4sHHIIHHII")
KEY = struct.Struct("<II")
UINT = struct.Struct("<I")
AUDIO_BLOCK_SIZE = 132


class FVDSFormatError(ValueError):
    pass


def require(condition, reason):
    if not condition:
        raise FVDSFormatError(reason)


def inspect_fvds(filename, hash_file=False, payload_hash=False):
    path = Path(filename)
    size = path.stat().st_size
    require(size >= HEADER.size, "Truncated FVDS header")
    with path.open("rb") as f:
        magic, width, height, fps_n, fps_d, rate, channels, frames, key_count = HEADER.unpack(
            f.read(HEADER.size))
        require(magic == b"FVDS", "Bad FVDS magic")
        require(0 < width <= 1024 and 0 < height <= 1024, "Bad video dimensions")
        require(0 < fps_n <= 1000 and 0 < fps_d <= 1000, "Bad frame rate")
        require(0 < rate <= 96000 and channels in (1, 2), "Bad audio configuration")
        require(0 < frames <= 108000, "Frame count beyond safety bound")
        require(0 < key_count <= frames, "Bad keyframe count")
        require(key_count <= 4096, "Keyframe index too large for NDS research")
        data_start = HEADER.size + key_count * KEY.size
        require(data_start + frames * UINT.size <= size, "Keyframes/frames exceed file bounds")
        keys = []
        prev_frame = prev_offset = -1
        for _ in range(key_count):
            frame, offset = KEY.unpack(f.read(KEY.size))
            require(prev_frame < frame < frames, "Keyframes must be sorted")
            require(prev_offset < offset < size and offset >= data_start,
                    "Keyframe offsets must be sorted and in file bounds")
            keys.append((frame, offset))
            prev_frame, prev_offset = frame, offset
        require(keys[0] == (0, data_start), "Missing first keyframe")
        expected_key_offsets = dict(keys)
        video_bytes = audio_bytes = audio_blocks_total = 0
        payload_digest = 2166136261
        payload_read = 0
        for frame_index in range(frames):
            position = f.tell()
            if frame_index in expected_key_offsets:
                require(position == expected_key_offsets[frame_index],
                        "Keyframe offset disagrees with packet boundary")
            require(size - position >= 4, "Truncated per-frame size field")
            value, = UINT.unpack(f.read(4))
            video_length, blocks = value & 0x1FFFF, value >> 17
            require(0 < blocks <= 255, "Bad per-frame ADPCM block count")
            require(video_length > 0 and video_length % 4 == 0,
                    "Bad video length/alignment")
            audio_length = blocks * channels * AUDIO_BLOCK_SIZE
            require(f.tell() + video_length + audio_length <= size,
                    "Video or audio packet exceeds file")
            if payload_hash:
                remaining=video_length+audio_length
                while remaining:
                    block=f.read(min(65536,remaining))
                    require(len(block)>0, "Truncated media payload")
                    for value in block:
                        payload_digest=((payload_digest ^ value)*16777619)&0xffffffff
                    payload_read+=len(block);remaining-=len(block)
            else:
                f.seek(video_length + audio_length, os.SEEK_CUR)
            video_bytes += video_length
            audio_bytes += audio_length
            audio_blocks_total += blocks
        require(f.tell() == size, "Unexpected trailing bytes")
    seconds = frames * fps_d / fps_n
    report = {
        "file": str(path), "size_bytes": size,
        "width": width, "height": height, "fps_numerator": fps_n,
        "fps_denominator": fps_d, "duration_seconds": seconds,
        "frames": frames, "audio_sample_rate": rate,
        "audio_channels": channels, "keyframe_count": key_count,
        "keyframes": [{"frame": n, "offset": offset} for n, offset in keys],
        "header_and_index_bytes": data_start,
        "frame_headers_bytes": frames * 4, "video_payload_bytes": video_bytes,
        "video_section_bytes": video_bytes + frames * 4,
        "audio_bytes": audio_bytes, "audio_blocks_per_channel": audio_blocks_total,
        "audio_capacity_samples_per_channel": audio_blocks_total * 256,
        "expected_audio_samples_per_channel": seconds * rate,
        "container_exact_bytes": data_start + frames * 4 + video_bytes + audio_bytes,
    }
    if payload_hash:
        report["payload_fnv1a32"]=f"{payload_digest:08X}"
        report["payload_read_bytes"]=payload_read
    if hash_file:
        digest = hashlib.sha256()
        with path.open("rb") as f:
            for part in iter(lambda: f.read(1024 * 1024), b""):
                digest.update(part)
        report["sha256"] = digest.hexdigest()
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("video", type=Path)
    p.add_argument("--sha256", action="store_true")
    p.add_argument("--payload-hash", action="store_true", help="Read all packet payload bytes and report on-device comparable FNV checksum")
    args = p.parse_args()
    try:
        print(json.dumps(inspect_fvds(args.video, args.sha256, args.payload_hash), indent=2, ensure_ascii=False))
    except (OSError, FVDSFormatError) as e:
        p.exit(2, "Invalid FVDS: %s\n" % e)


if __name__ == "__main__":
    main()
