"""Original CPyMO NDS NVL1/NVA1 encoder; no external proprietary codec tables.

FFmpeg supplies normalized RGB and PCM. We only package bounded 256x192
RGB555 LZ4/XOR pictures plus 22,050-Hz mono samples in the format understood
by cpymo-backends/nds/source/movie-lz4-r7.h. The source is never modified.
"""
from __future__ import annotations
from pathlib import Path
import functools
import hashlib
import os
import re
import shutil
import struct
import subprocess
import tempfile
import zlib

import lz4.block
import lossless

WIDTH, HEIGHT, RATE = 256, 192, 22050
RAW_RGB = WIDTH * HEIGHT * 3
RAW_FRAME = WIDTH * HEIGHT * 2
VIDEO_SUFFIXES = frozenset({'.mp4', '.avi', '.mpg', '.mpeg', '.mkv', '.webm',
                            '.3gp', '.wmv', '.mov', '.ogv', '.flv', '.m4v'})


@functools.lru_cache(maxsize=8)
def validate_ffmpeg(ffmpeg):
    """Reject old Starter's *audio-only* FFmpeg before producing a broken video.

    Probe real filter/decoder tables, not a version string. ffmpeg -version
    succeeds even when video codecs and swscale were intentionally disabled.
    This check is required for video conversions and Windows distribution.
    """
    executable = str(ffmpeg)
    flags = 0x08000000 if os.name == 'nt' else 0
    def capabilities(option):
        try:
            done = subprocess.run(
                [executable, '-hide_banner', option],
                capture_output=True, text=True, encoding='utf-8',
                errors='replace', timeout=20, creationflags=flags)
        except (OSError, subprocess.TimeoutExpired) as error:
            raise ValueError('Video-capable FFmpeg is unavailable: '+str(error)) from error
        if done.returncode != 0:
            raise ValueError('FFmpeg cannot list '+option+' capabilities: '+done.stderr[-400:])
        # FFmpeg builds/terminal platforms sometimes put capability tables on
        # stderr. Collect both while keeping the check fully read-only.
        return done.stdout + '\n' + done.stderr
    filters = capabilities('-filters')
    decoders = capabilities('-decoders')
    required = ('fps', 'scale', 'pad', 'format', 'aresample', 'apad', 'atrim')
    # Do NOT match a fixed-width flag prefix: FFmpeg has changed its filter
    # capability indicators across releases (e.g. three vs five columns).
    # Instead identify named filters followed by their V->V/A->A signature.
    available = set()
    for line in filters.splitlines():
        fields = line.split()
        if len(fields) >= 3 and '->' in fields[2]:
            available.add(fields[1])
    missing = sorted(set(required) - available)
    video_decoders = set(re.findall(r'^\s*[A-Z.]{6}\s+([a-z0-9_]+)\s', decoders, re.M))
    if not video_decoders.intersection({'h264', 'hevc', 'mpeg4', 'mjpeg', 'vp9', 'av1'}):
        missing.append('MP4/video decoder')
    if missing:
        raise ValueError(
            'This FFmpeg was built for audio only, or lacks NDS video features: '+
            ', '.join(missing)+
            '. Use a video-capable FFmpeg build; the old Starter audio-only ffmpeg.exe cannot convert movies.')
    return {'filters':sorted(set(required)), 'video_decoder':True}


def _rgb555(rgb: bytes) -> bytes:
    if len(rgb) != RAW_RGB:
        raise ValueError('Truncated RGB24 movie frame')
    out = bytearray(RAW_FRAME)
    j = 0
    for i in range(0, RAW_RGB, 3):
        pixel = 0x8000 | (rgb[i] >> 3) | ((rgb[i+1] >> 3) << 5) | ((rgb[i+2] >> 3) << 10)
        out[j] = pixel & 255
        out[j+1] = pixel >> 8
        j += 2
    return bytes(out)


def encode(source, destination, ffmpeg='ffmpeg', *, fps=12, audio='pcm'):
    """Write player-compatible .nvl to a new file in the caller's staging area."""
    source, destination = Path(source).resolve(), Path(destination).resolve()
    if not source.is_file() or source.suffix.lower() not in VIDEO_SUFFIXES:
        raise ValueError('Video input must be a supported existing media file')
    if source == destination or destination.exists():
        raise ValueError('Video output must be a new file distinct from source')
    if audio not in ('pcm', 'nar') or fps not in (6, 8, 12, 15, 20, 24):
        raise ValueError('Unsupported NDS movie FPS/audio option')
    ffmpeg = str(ffmpeg)
    validate_ffmpeg(ffmpeg)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='nvl-', dir=destination.parent) as temp:
        temp = Path(temp)
        video_stream = temp/'frames.bin'
        pcm = temp/'sound.pcm'
        # Decode one RGB24 frame at a time; memory independent of duration.
        filterchain = (f'fps={fps},scale={WIDTH}:{HEIGHT}:'
                       'force_original_aspect_ratio=decrease:flags=bicubic,'
                       f'pad={WIDTH}:{HEIGHT}:(ow-iw)/2:(oh-ih)/2:black,format=rgb24')
        command = [ffmpeg, '-nostdin', '-hide_banner', '-loglevel', 'error',
                   '-i', str(source), '-map', '0:v:0', '-an',
                   '-vf', filterchain, '-f', 'rawvideo', '-pix_fmt', 'rgb24', 'pipe:1']
        frames = 0
        previous = None
        with video_stream.open('wb') as out:
            process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                       creationflags=0x08000000 if os.name == 'nt' else 0)
            try:
                while True:
                    raw = process.stdout.read(RAW_RGB)
                    if not raw:
                        break
                    if len(raw) != RAW_RGB:
                        raise ValueError('Truncated source video frame')
                    if frames >= fps*600:
                        raise ValueError('NDS video longer than 600 seconds')
                    frame = _rgb555(raw)
                    key = previous is None or frames % (fps*4) == 0
                    delta = frame if key else bytes(a ^ b for a,b in zip(frame,previous))
                    packed = lz4.block.compress(delta, mode='high_compression',
                                                compression=6, store_size=False)
                    use_packed = len(packed) < RAW_FRAME
                    payload = packed if use_packed else delta
                    out.write(struct.pack('<III', len(payload),
                              (1 if use_packed else 0) | (0 if key else 2),
                              zlib.adler32(frame)))
                    out.write(payload)
                    previous = frame
                    frames += 1
                stderr = process.stderr.read()
                if process.wait() != 0:
                    raise ValueError('FFmpeg video decoding failed: '+
                                     stderr.decode('utf-8','replace')[-1000:])
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
                process.stdout.close()
                process.stderr.close()
        if not frames:
            raise ValueError('Video contains no decodable frames')
        samples = frames*RATE//fps
        # Force resampling BEFORE atrim; otherwise source rate (e.g. 44.1k)
        # can cause the output to have only half the expected sample count.
        audio_command = [ffmpeg, '-nostdin', '-hide_banner', '-loglevel', 'error',
                         '-i', str(source), '-map', '0:a:0?', '-vn', '-ac', '1',
                         '-ar', str(RATE), '-af',
                         f'aresample={RATE},apad,atrim=end_sample={samples}',
                         '-f', 's16le', str(pcm)]
        result = subprocess.run(audio_command, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                creationflags=0x08000000 if os.name == 'nt' else 0)
        if result.returncode:
            # FFmpeg spelling differs by version. Accept only the explicit
            # "no audio stream" case; real decoding errors must fail the job.
            if (b'Output file does not contain any stream' not in result.stderr
                    and b'Output file #0 does not contain any stream' not in result.stderr):
                raise ValueError('FFmpeg audio decoding failed: '+
                                 result.stderr.decode('utf-8','replace')[-1000:])
            with pcm.open('wb') as out:
                chunk = b'\x00'*8192
                remaining = samples*2
                while remaining:
                    n = min(len(chunk),remaining)
                    out.write(chunk[:n])
                    remaining -= n
        if pcm.stat().st_size != samples*2:
            raise ValueError('Audio sample count did not match video playback clock')
        data = pcm
        if audio == 'nar':
            data = temp/'sound.nar'
            audit = lossless.encode_audio(pcm, data)
            if audit['samples'] != samples:
                raise ValueError('NAR2 sample count changed')
        start = 32 + video_stream.stat().st_size
        if start >= 512*1024*1024:
            raise ValueError('NDS video packets exceed 512 MiB limit')
        magic = b'NVA1' if audio=='nar' else b'NVL1'
        try:
            with destination.open('xb') as out:
                out.write(struct.pack('<4s7I',magic,WIDTH,HEIGHT,
                                      fps,frames,RATE,samples,start))
                with video_stream.open('rb') as inp:
                    shutil.copyfileobj(inp,out,1024*1024)
                with data.open('rb') as inp:
                    shutil.copyfileobj(inp,out,1024*1024)
        except BaseException:
            destination.unlink(missing_ok=True)
            raise
    # Hash in chunks, including for long full-length clips.
    h=hashlib.sha256()
    with destination.open('rb') as inp:
        for block in iter(lambda:inp.read(1024*1024),b''):
            h.update(block)
    return {'format':magic.decode('ascii'),'frames':frames,'fps':fps,
            'duration_seconds':frames/fps,'audio_samples':samples,'audio_rate':RATE,
            'width':WIDTH,'height':HEIGHT,'bytes':destination.stat().st_size,
            'sha256':h.hexdigest(),'has_audio':result.returncode==0}
