from array import array
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))


class AudioTests(unittest.TestCase):
    def testFloatStereoRoundTripAndRejectTruncated(self):
        from audio import readWav, writeWav
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'audio.wav'
            samples = array('f', [0.0, -0.0, 0.25, -0.5, 1.0, -1.0])
            writeWav(path, samples, 48000, 2)
            output, rate, channels = readWav(path)
            self.assertEqual(output.tobytes(), samples.tobytes())
            self.assertEqual((rate, channels), (48000, 2))
            path.write_bytes(path.read_bytes()[:-1])
            with self.assertRaises(ValueError):
                readWav(path)

    def testMonoCancellationAndRawGainDifference(self):
        from compare import compareSamples
        samples = array('f', [0.25, -0.25] * 64)
        report = compareSamples(samples, array('f', [value * 2 for value in samples]), 48000)
        self.assertEqual(report['a']['mono']['rms'], 0.0)
        self.assertEqual(report['a']['stereoCorrelation'], -1.0)
        self.assertAlmostEqual(report['difference']['left']['rms'], 0.25)
        self.assertAlmostEqual(report['difference']['left']['gainDb'], 6.020599913279624)
        with self.assertRaises(ValueError):
            compareSamples(samples, samples[:-2], 48000)


if __name__ == '__main__':
    unittest.main()
