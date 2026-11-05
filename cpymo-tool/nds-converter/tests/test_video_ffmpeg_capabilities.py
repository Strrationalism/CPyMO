"""1.2 packaging gate: 1.1 Starter's audio-only FFmpeg must never ship as video-capable."""
import subprocess
import sys
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import video_nvl


class VideoFFmpegCapabilityTests(unittest.TestCase):
    def setUp(self):
        video_nvl.validate_ffmpeg.cache_clear()

    def test_audio_only_starter_rejected_even_when_version_succeeds(self):
        filters=' ... aresample A->A\n ... atrim A->A\n'
        decoders=' A....D mp3   MP3 audio\n'
        def output(argv,**kwargs):
            value=filters if argv[-1]=='-filters' else decoders
            return subprocess.CompletedProcess(argv,0,stdout=value,stderr='')
        with mock.patch.object(video_nvl.subprocess,'run',side_effect=output):
            with self.assertRaisesRegex(ValueError,'audio only'):
                video_nvl.validate_ffmpeg('ffmpeg-audio.exe')

    def test_capable_ffmpeg_accepted_and_cached(self):
        names='fps scale pad format aresample apad atrim'.split()
        filters=''.join(f' ... {name} V->V\n' for name in names)
        decoders=' VFS..D h264 H.264 decoder\n'
        calls=[]
        def output(argv,**kwargs):
            calls.append(argv[-1])
            return subprocess.CompletedProcess(argv,0,
                stdout=filters if argv[-1]=='-filters' else decoders,stderr='')
        with mock.patch.object(video_nvl.subprocess,'run',side_effect=output):
            result=video_nvl.validate_ffmpeg('known-full-ffmpeg.exe')
            self.assertEqual(result['video_decoder'],True)
            self.assertEqual(video_nvl.validate_ffmpeg('known-full-ffmpeg.exe'),result)
            self.assertEqual(calls,['-filters','-decoders'])

    def test_ffmpeg_new_filter_flag_width_and_stderr_tables(self):
        # New FFmpeg variants expose more flag columns, and some platforms
        # send the capability listing to stderr rather than stdout.
        names='fps scale pad format aresample apad atrim'.split()
        table=''.join(f' TSC.. {name} V->V supported\n' for name in names)
        def output(argv,**kwargs):
            if argv[-1]=='-filters':
                return subprocess.CompletedProcess(argv,0,stdout='',stderr=table)
            return subprocess.CompletedProcess(argv,0,
                stdout=' VFS..D h264 supported\n',stderr='')
        with mock.patch.object(video_nvl.subprocess,'run',side_effect=output):
            self.assertTrue(video_nvl.validate_ffmpeg('ffmpeg8.exe')['video_decoder'])

    def test_bad_executable_or_timeout_fails_before_output_creation(self):
        with mock.patch.object(video_nvl.subprocess,'run',side_effect=OSError('missing')):
            with self.assertRaisesRegex(ValueError,'unavailable'):
                video_nvl.validate_ffmpeg('missing-ffmpeg.exe')
        video_nvl.validate_ffmpeg.cache_clear()
        with mock.patch.object(video_nvl.subprocess,'run',
                               side_effect=subprocess.TimeoutExpired('ffmpeg',20)):
            with self.assertRaisesRegex(ValueError,'unavailable'):
                video_nvl.validate_ffmpeg('stalled-ffmpeg.exe')


if __name__=='__main__':
    unittest.main()
