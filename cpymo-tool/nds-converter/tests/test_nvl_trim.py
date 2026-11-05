"""Host regression for NVL1 beginning-only trimming (no media assets)."""
import struct, sys, tempfile, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from nvl_trim import trim_first_frames, NVLFormatError

def fixture():
    packets=[struct.pack("<3I",4,0 if i==0 else 2,123+i)+bytes([i+1])*4
             for i in range(6)]
    audio=bytes(range(256))* (11025*2//256)
    # 6 frames @ 12 fps is half a second -> 11025 samples
    audio=audio[:11025*2].ljust(11025*2,b"\0")
    return struct.pack("<4s7I",b"NVL1",256,192,12,6,22050,11025,32+6*16)+b"".join(packets)+audio

class NVLTrimTest(unittest.TestCase):
    def test_clip_copy_exact(self):
        with tempfile.TemporaryDirectory() as tmp:
            source=Path(tmp)/"src.nvl";target=Path(tmp)/"short.nvl"
            data=fixture();source.write_bytes(data)
            info=trim_first_frames(source,target,3)
            trimmed=target.read_bytes()
            self.assertEqual(info["frames"],3)
            self.assertEqual(trimmed[32:32+48],data[32:32+48])
            self.assertEqual(trimmed[80:],data[32+6*16:32+6*16+5512*2])
            self.assertEqual(struct.unpack("<4s7I",trimmed[:32])[4],3)
            self.assertEqual(len(trimmed),32+48+5512*2)
            self.assertEqual(source.read_bytes(),data)

    def test_trailing_source_audio_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            inp=Path(tmp)/"bad.nvl";inp.write_bytes(fixture()+b"x")
            with self.assertRaises(NVLFormatError):
                trim_first_frames(inp,Path(tmp)/"clip.nvl",3)

    def test_no_overwrite(self):
        with tempfile.TemporaryDirectory() as tmp:
            inp=Path(tmp)/"a.nvl";inp.write_bytes(fixture())
            with self.assertRaises(ValueError):
                trim_first_frames(inp,inp,3)

if __name__=="__main__":unittest.main()
