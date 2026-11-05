"""Validate host WAV output from the same FVDS packet and stereo ADPCM
decoder C sources intended for CPyMO. Synthetic samples only; no game data.
"""
from pathlib import Path
import shutil,subprocess,tempfile,unittest,struct,wave
from test_fvds_inspect import sample
ROOT=Path(__file__).resolve().parents[3]
SRC=ROOT/"cpymo-backends/nds/source"
EXTRACT=ROOT/"cpymo-backends/nds/tools/fvds-to-wav.c"

@unittest.skipUnless(shutil.which("gcc"),"GCC unavailable")
class FVDSWAVTests(unittest.TestCase):
    def test_synthetic_export_and_bad_audio(self):
        with tempfile.TemporaryDirectory() as td:
            d=Path(td);exe=d/"extract"
            subprocess.run(["gcc","-std=c11","-O2","-Wall","-Wextra","-Werror",
                            "-fsanitize=undefined","-I",str(SRC),
                            str(SRC/"fvds-stream-nds.c"),str(SRC/"fvds-adpcm-nds.c"),
                            str(EXTRACT),"-o",str(exe)],check=True)
            input_file=d/"sample.fv";output_file=d/"test.wav"
            input_file.write_bytes(sample())
            subprocess.run([str(exe),str(input_file),str(output_file)],
                           capture_output=True,text=True,check=True)
            with wave.open(str(output_file),"rb") as wav:
                self.assertEqual(wav.getnchannels(),2)
                self.assertEqual(wav.getframerate(),47605)
                self.assertEqual(wav.getsampwidth(),2)
                self.assertEqual(wav.getnframes(),47*256)
                pcm=wav.readframes(47*256)
                self.assertEqual(pcm,bytes(47*256*4))
            self.assertEqual(output_file.stat().st_size,44+47*256*4)
            # Do not allow the host exporter to clobber the source movie.
            before=input_file.read_bytes()
            unsafe=subprocess.run([str(exe),str(input_file),str(input_file)],
                                  capture_output=True,text=True)
            self.assertEqual(unsafe.returncode,2)
            self.assertEqual(input_file.read_bytes(),before)
            alias=d/"movie-alias.fv"
            alias.symlink_to(input_file)
            unsafe=subprocess.run([str(exe),str(input_file),str(alias)],
                                  capture_output=True,text=True)
            self.assertEqual(unsafe.returncode,2)
            self.assertEqual(input_file.read_bytes(),before)
            output_file.unlink()
            input_file.write_bytes(sample(tail=True))
            broken=subprocess.run([str(exe),str(input_file),str(output_file)],
                                  capture_output=True,text=True)
            self.assertNotEqual(broken.returncode,0)
            self.assertFalse(output_file.exists())
            payload=bytearray(sample())
            # first packet starts after 28-byte header + 8-byte key index;
            # first stereo block begins after 4-byte packet + 8-byte video.
            struct.pack_into("<H",payload,36+4+8+2,89)
            input_file.write_bytes(payload)
            broken=subprocess.run([str(exe),str(input_file),str(output_file)],
                                  capture_output=True,text=True)
            self.assertNotEqual(broken.returncode,0)
            self.assertFalse(output_file.exists())

if __name__=="__main__":
    unittest.main()
