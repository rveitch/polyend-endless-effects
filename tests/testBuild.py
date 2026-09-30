"""Exercise real build selection, manifests and legacy entry points."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
TOOLCHAIN = os.environ.get('TOOLCHAIN', '/Users/ryanveitch/nodejs/polyend/agt15-2/bin/arm-none-eabi-')


class BuildTests(unittest.TestCase):
    def command(self, *args, root=ROOT):
        return subprocess.run([sys.executable, str(root / 'scripts/effects.py'), *args], cwd=root,
                              capture_output=True, text=True, timeout=120)

    def testCatalogAndAllHostRegressions(self):
        result = self.command('list')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('junoChorus', result.stdout)
        self.assertIn('passthrough', result.stdout)
        result = self.command('test', '--effect', 'all')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('vintage candidate regressions', result.stdout)
        self.assertIn('channels preserved bit-for-bit', result.stdout)

    def testUnknownEffectFailsBeforeCreatingBuild(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / 'neverCreated'
            result = self.command('build', '--effect', '../junoChorus', '--build-dir', str(output))
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('effect', result.stderr.lower())
            self.assertFalse(output.exists())

    @unittest.skipUnless(Path(TOOLCHAIN + 'g++').exists() or shutil.which(TOOLCHAIN + 'g++'), 'ARM toolchain unavailable')
    def testFixtureSelectionDependenciesAndManifest(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / 'repo'
            root.mkdir()
            for name in ['scripts', 'buildSupport', 'effects', 'vendor']:
                shutil.copytree(ROOT / name, root / name)
            shutil.copy2(ROOT / 'sdk.lock.json', root)
            catalogPath = root / 'effects/catalog.json'
            catalog = json.loads(catalogPath.read_text())
            fixture = root / 'effects/fixture'
            fixture.mkdir()
            source = fixture / 'PatchImpl.cpp'
            shared = root / 'shared'
            shared.mkdir()
            (shared / 'Gain.h').write_text('#pragma once\n#include "Gain.inc"\n')
            (shared / 'Gain.inc').write_text('static constexpr float fixtureGain = 1.0f;\n')
            source.write_text('#include "../../shared/Gain.h"\n' + (root / 'effects/passthrough/PatchImpl.cpp').read_text())
            (fixture / 'README.md').write_text('Temporary third effect.\n')
            catalog['effects'].append({'id': 'fixture', 'name': 'Fixture', 'source': 'effects/fixture/PatchImpl.cpp',
                                       'documentation': 'effects/fixture/README.md', 'tests': []})
            catalogPath.write_text(json.dumps(catalog))
            output = root / 'build/fixture'
            args = ['build', '--effect', 'fixture', '--build-dir', str(output), '--toolchain', TOOLCHAIN]
            result = self.command(*args, root=root)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            manifestPath = next(output.glob('*.manifest.json'))
            manifest = json.loads(manifestPath.read_text())
            self.assertEqual(manifest['effectId'], 'fixture')
            self.assertEqual(manifest['sdkCommit'], '708f08d7c8e365b8a153c66e0e5200fbdaff1ce0')
            self.assertIn('effects/fixture/PatchImpl.cpp', manifest['sourceHashes'])
            self.assertFalse(any('junoChorus/PatchImpl.cpp' in p for p in manifest['sourceHashes']))
            self.assertIn('shared/Gain.h', manifest['sourceHashes'])
            self.assertIn('shared/Gain.inc', manifest['sourceHashes'])
            effectObject = output / 'objects/fixture/effects/fixture/PatchImpl.o'
            initial = effectObject.stat().st_mtime_ns
            time.sleep(1.1)
            header = root / 'vendor/FxPatchSDK/source/Patch.h'
            os.utime(header, None)  # Unchanged contents: exercise dependency tracking without dirtying SDK.
            result = self.command(*args, root=root)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertGreater(effectObject.stat().st_mtime_ns, initial)
            initial = effectObject.stat().st_mtime_ns
            time.sleep(1.1)
            result = self.command(*args, '--extra-flags=-DFIXTURE_REBUILD=1', root=root)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertGreater(effectObject.stat().st_mtime_ns, initial)
            from hashlib import sha256
            self.assertEqual(sha256(Path(manifest['elf']).read_bytes()).hexdigest(), manifest['elfHash'])

    @unittest.skipUnless(Path(TOOLCHAIN + 'g++').exists() or shutil.which(TOOLCHAIN + 'g++'), 'ARM toolchain unavailable')
    def testArtifactHistoryAndSameTimestampChanges(self):
        from hashlib import sha256
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / 'repo'
            root.mkdir()
            for name in ['scripts', 'buildSupport', 'effects', 'vendor']:
                shutil.copytree(ROOT / name, root / name)
            shutil.copy2(ROOT / 'sdk.lock.json', root)
            source = root / 'effects/passthrough/PatchImpl.cpp'
            source.write_text('#include "Gain.h"\n' + source.read_text().replace(
                'void processAudio(std::span<float> /* left */, std::span<float> /* right */) override {}',
                'void processAudio(std::span<float> left, std::span<float> right) override { for (float& v : left) v *= gain; for (float& v : right) v *= gain; }'))
            header = source.parent / 'Gain.h'
            header.write_text('static constexpr float gain = 0.25f;\n')
            args = ['build', '--effect', 'passthrough', '--toolchain', TOOLCHAIN]
            first = self.command(*args, root=root)
            self.assertEqual(first.returncode, 0, first.stderr)
            output = root / 'build/passthrough'
            manifest = json.loads(next(output.glob('*.manifest.json')).read_text())
            effectObject = output / 'objects/passthrough/effects/passthrough/PatchImpl.o'
            original = sha256(effectObject.read_bytes()).hexdigest()
            header.write_text('static constexpr float gain = 0.75f;\n')
            os.utime(header, ns=(effectObject.stat().st_mtime_ns, effectObject.stat().st_mtime_ns))
            second = self.command(*args, root=root)
            self.assertEqual(second.returncode, 0, second.stderr)
            with self.subTest(check='contentInvalidation'):
                self.assertNotEqual(original, sha256(effectObject.read_bytes()).hexdigest())
            third = self.command(*args, '--extra-flags=-g0', root=root)
            self.assertEqual(third.returncode, 0, third.stderr)
            with self.subTest(check='immutableElf'):
                self.assertEqual(sha256(Path(manifest['elf']).read_bytes()).hexdigest(), manifest['elfHash'])

    def testLegacyHostCommands(self):
        with tempfile.TemporaryDirectory() as temporary:
            for source in ['example/tests/ChorusMixTest.cpp', 'example/tests/VintageCandidateTest.cpp']:
                binary = Path(temporary) / Path(source).stem
                result = subprocess.run(['c++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', source,
                                         '-o', str(binary)], cwd=ROOT, capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stderr)
                result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run(['make', '-C', 'example', '-f', 'diagnostic/Makefile', 'test',
                                     'BUILD_DIR=' + temporary], cwd=ROOT, capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
