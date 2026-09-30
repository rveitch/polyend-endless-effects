from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))


class EndlTests(unittest.TestCase):
    def inspect(self, data):
        from endl import inspectImage
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'fixture.endl'
            path.write_bytes(data)
            return inspectImage(path, ROOT, 0x80000000)

    def fixture(self):
        # Independent ABI fixture: 136-byte header, 64-byte code, then 8 bytes BSS.
        return bytearray(struct.pack('<IHH13I3I16I', 0x48435450, 11, 0,
                         *([0x80000089] * 13), 64, 0x800000C8, 8, *([0] * 16)) + bytes(64))

    def testValidImageAndOptionalCallback(self):
        data = self.fixture()
        struct.pack_into('<I', data, 40, 0)
        result = self.inspect(data)
        self.assertEqual(result['fileBytes'], 200)
        self.assertEqual(result['ramBytes'], 208)

    def testTruncationBadPointersAndBssAreRejected(self):
        with self.assertRaises(ValueError):
            self.inspect(self.fixture()[:-1])
        for offset, value in [(0, 0), (4, 12), (8, 0), (12, 0x80000088),
                              (12, 0x80001001), (64, 0x80000088), (68, 524288)]:
            data = self.fixture()
            struct.pack_into('<I' if offset != 4 else '<H', data, offset, value)
            with self.subTest(offset=offset, value=value), self.assertRaises(ValueError):
                self.inspect(data)


if __name__ == '__main__':
    unittest.main()
