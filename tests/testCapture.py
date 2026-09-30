from array import array
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))


class CaptureTests(unittest.TestCase):
    def command(self, *args):
        return subprocess.run([sys.executable, str(ROOT / 'scripts/capture.py'), *args], cwd=ROOT,
                              capture_output=True, text=True, timeout=60)

    def testDistinctStereoPassthroughAndMetadata(self):
        from audio import writeWav, readWav
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            source = folder / 'input.wav'
            samples = array('f', [0.0, -0.0, 0.125, -0.25, 0.5, -0.75] * 101)
            writeWav(source, samples, 48000, 2)
            output = folder / 'output.wav'
            result = self.command('--effect', 'passthrough', '--input', str(source), '--output', str(output), '--block-size', '7')
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            actual, _, _ = readWav(output)
            self.assertEqual(actual.tobytes(), samples.tobytes())
            metadata = json.loads(output.with_suffix('.json').read_text())
            self.assertEqual(metadata['channels'], 2)
            self.assertEqual(metadata['frames'], 303)
            self.assertEqual(metadata['callbackSize'], 7)
            self.assertIn('sdkCommit', metadata)

    def testScheduledActionsAreCallbackIndependent(self):
        from audio import readWav
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            events = folder / 'events.json'
            events.write_text(json.dumps([{'frame': 1500, 'action': 0}, {'frame': 1733, 'action': 1},
                                         {'frame': 1891, 'parameter': 1, 'value': 0.9}]))
            paths = [folder / 'a.wav', folder / 'b.wav']
            for block, path in zip([8, 127], paths):
                result = self.command('--effect', 'junoChorus', '--signal', 'broadband', '--seconds', '0.1',
                                      '--seed', '42', '--events', str(events), '--block-size', str(block), '--output', str(path))
                self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(readWav(paths[0])[0].tobytes(), readWav(paths[1])[0].tobytes())

    def testInvalidParametersAndExistingOutputAreRejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'capture.wav'
            result = self.command('--param0', 'nan', '--output', str(path))
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(path.exists())
            path.write_bytes(b'preserve me')
            result = self.command('--seconds', '0.01', '--output', str(path))
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(path.read_bytes(), b'preserve me')

    def testCompareRejectsSidecarMismatch(self):
        from audio import writeWav
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            a = folder / 'a.wav'
            b = folder / 'b.wav'
            samples = array('f', [0.1, -0.1] * 64)
            for path in [a, b]:
                writeWav(path, samples, 48000, 2)
                path.with_suffix('.json').write_text(json.dumps({'sampleRate': 44100, 'channels': 2, 'frames': 64}))
            result = subprocess.run([sys.executable, str(ROOT / 'scripts/compare.py'), str(a), str(b)],
                                    capture_output=True, text=True, timeout=20)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('metadata', result.stderr.lower())


if __name__ == '__main__':
    unittest.main()
