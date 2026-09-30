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


class ComparisonIntegrityTests(unittest.TestCase):
    def fixture(self, folder):
        from audio import writeWav
        paths = [folder / 'a.wav', folder / 'b.wav']
        for path in paths:
            writeWav(path, array('f', [0.1, -0.1] * 64))
        return paths

    def testOutputCannotReplaceListeningWav(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            paths = self.fixture(folder)
            preview = folder / 'preview'
            for filename in ('a.listening.wav', 'transforms.json'):
                with self.subTest(filename=filename):
                    result = subprocess.run([sys.executable, str(ROOT / 'scripts/compare.py'), *map(str, paths),
                                             '--preview-dir', str(preview), '--output', str(preview / filename)],
                                            capture_output=True, text=True, timeout=20)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertFalse(preview.exists())

    def testMalformedSidecarIsRejected(self):
        for metadata in [[], {'schemaVersion': 999, 'sampleRate': 48000, 'channels': 2, 'frames': 64},
                         {'sampleRate': 48000, 'channels': 2, 'frames': 64, 'durationSeconds': 2},
                         {'sampleRate': 48000, 'channels': 2, 'frames': 64, 'events': [{'frame': 65, 'action': 0}]}]:
            with self.subTest(metadata=metadata), tempfile.TemporaryDirectory() as temporary:
                paths = self.fixture(Path(temporary))
                paths[0].with_suffix('.json').write_text(json.dumps(metadata))
                result = subprocess.run([sys.executable, str(ROOT / 'scripts/compare.py'), *map(str, paths)],
                                        capture_output=True, text=True, timeout=20)
                self.assertNotEqual(result.returncode, 0)


class HostDependencyTests(unittest.TestCase):
    def testIncludedSharedDataIsCapturedAndListeningCopiesPreserveRaw(self):
        from audio import readWav, writeWav
        from capture import renderRaw
        from project import hashFile
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            source = folder / 'PatchImpl.cpp'
            header = folder / 'Gain.h'
            data = folder / 'Gain.inc'
            header.write_text('#include "Gain.inc"\n')
            data.write_text('static constexpr float gain = 0.25f;\n')
            source.write_text('#include "Gain.h"\n' + (ROOT / 'effects/passthrough/PatchImpl.cpp').read_text().replace(
                '../../vendor/FxPatchSDK/source/Patch.h', str(ROOT / 'vendor/FxPatchSDK/source/Patch.h')).replace(
                'void processAudio(std::span<float> /* left */, std::span<float> /* right */) override {}',
                'void processAudio(std::span<float> left, std::span<float> right) override { for (float& v : left) v *= gain; for (float& v : right) v *= gain; }'))
            samples = array('f', [0.25, -0.125] * 64)
            raw = folder / 'raw.wav'
            other = folder / 'other.wav'
            build = renderRaw(source, samples, [None, None, None], [], 7, raw)
            self.assertEqual(build['sourceHashes'][str(data.resolve())], hashFile(data))
            self.assertEqual(build['sourceHashes'][str(header.resolve())], hashFile(header))
            self.assertEqual(readWav(raw)[0], array('f', [value * 0.25 for value in samples]))
            writeWav(other, samples)
            before = hashFile(raw)
            result = subprocess.run([sys.executable, str(ROOT / 'scripts/compare.py'), str(raw), str(other),
                                     '--preview-dir', str(folder / 'preview'), '--shift-b', '3'],
                                    capture_output=True, text=True, timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(hashFile(raw), before)
            self.assertAlmostEqual(json.loads(result.stdout)['difference']['left']['gainDb'], 12.041199826559248)
            transform = json.loads((folder / 'preview/transforms.json').read_text())
            self.assertEqual(transform['transformations'][1]['shiftFrames'], 3)


if __name__ == '__main__':
    unittest.main()
