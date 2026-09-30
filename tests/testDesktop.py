from pathlib import Path
import subprocess
import sys
import unittest
ROOT = Path(__file__).resolve().parents[1]

class DesktopCliTests(unittest.TestCase):
    def testUnknownEffectRejectedBeforeConfigure(self):
        result = subprocess.run([sys.executable, str(ROOT / 'scripts/desktop.py'), 'build', '--effect', '../unknown'],
                                cwd=ROOT, capture_output=True, text=True, timeout=20)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Unknown effect', result.stderr)

if __name__ == '__main__':
    unittest.main()
