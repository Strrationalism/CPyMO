"""End-to-end synthetic-media proof for independent NVL1/NVA1 FFmpeg encoder."""
import hashlib
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib
from pathlib import Path
import lz4.block

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import video_nvl
FFMPEG=shutil.which('ffmpeg')


@unittest.skipUnless(FFMPEG,'FFmpeg required')
class NVLVideoTests(unittest.TestCase):
    def make_movie(self,where,with_audio=True):
        dest=where/'sample.mp4'
        cmd=[FFMPEG,'-nostdin','-loglevel','error','-f','lavfi','-i',
             'testsrc2=duration=1.3:size=320x240:rate=15']
        if with_audio:
            cmd+=['-f','lavfi','-i','sine=frequency=440:duration=1.3:sample_rate=44100']
        cmd+=['-pix_fmt','yuv420p','-y',str(dest)]
        subprocess.run(cmd,capture_output=True,check=True)
        return dest

    def check_frames(self,video):
        data=video.read_bytes()
        magic,w,h,fps,frames,rate,samples,audio_at=struct.unpack_from('<4s7I',data)
        self.assertIn(magic,(b'NVL1',b'NVA1'))
        self.assertEqual((w,h,rate,samples),(256,192,22050,frames*22050//fps))
        previous=None;pos=32
        for frame_id in range(frames):
            n,flag,expected=struct.unpack_from('<III',data,pos);pos+=12
            self.assertLessEqual(n,98304)
            packet=data[pos:pos+n];pos+=n
            delta=lz4.block.decompress(packet,uncompressed_size=98304) if flag&1 else packet
            picture=(bytes(a^b for a,b in zip(previous,delta)) if flag&2 else delta)
            self.assertEqual(len(picture),98304)
            self.assertEqual(zlib.adler32(picture),expected)
            self.assertTrue(all(picture[i]&0x80 for i in range(1,98304,2)))
            if frame_id==0:self.assertFalse(flag&2)
            previous=picture
        self.assertEqual(pos,audio_at)
        if magic==b'NVL1':self.assertEqual(len(data)-pos,samples*2)
        else:self.assertEqual(data[pos:pos+4],b'NAR2')
        return frames

    def test_ordinary_source_to_player_packets_and_original_unchanged(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);src=self.make_movie(base)
            before=hashlib.sha256(src.read_bytes()).hexdigest()
            for form,expected_magic in (('pcm',b'NVL1'),('nar',b'NVA1')):
                dest=base/(form+'.nvl')
                meta=video_nvl.encode(src,dest,FFMPEG,audio=form)
                self.assertEqual(meta['format'],expected_magic.decode())
                self.assertEqual(meta['frames'],self.check_frames(dest))
                self.assertEqual(meta['sha256'],hashlib.sha256(dest.read_bytes()).hexdigest())
            self.assertEqual(before,hashlib.sha256(src.read_bytes()).hexdigest())

    def test_video_without_audio_outputs_silence(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);src=self.make_movie(base,False)
            dst=base/'silent.nvl';meta=video_nvl.encode(src,dst,FFMPEG)
            self.check_frames(dst)
            self.assertFalse(meta['has_audio'])
            data=dst.read_bytes();offset=struct.unpack_from('<I',data,28)[0]
            self.assertEqual(set(data[offset:]),{0})

    def test_reject_unsupported_invalid_input_without_overwriting(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);src=base/'fake.mp4';src.write_bytes(b'broken')
            dest=base/'bad.nvl'
            with self.assertRaises(ValueError):video_nvl.encode(src,dest,FFMPEG)
            self.assertFalse(dest.exists())
            real=self.make_movie(base)
            dest.write_bytes(b'keep')
            with self.assertRaises(ValueError):video_nvl.encode(real,dest,FFMPEG)
            self.assertEqual(dest.read_bytes(),b'keep')


if __name__=='__main__':unittest.main()
