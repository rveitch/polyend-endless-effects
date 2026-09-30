import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()


class SdkTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / 'project'
        self.root.mkdir()
        for name in ['scripts', 'vendor']:
            shutil.copytree(ROOT / name, self.root / name)
        shutil.copy2(ROOT / 'sdk.lock.json', self.root)
        self.remote = Path(self.temporary.name) / 'upstream'
        shutil.copytree(ROOT / 'vendor/FxPatchSDK', self.remote)
        git(self.remote, 'init', '-q')
        git(self.remote, 'config', 'user.name', 'SDK fixture')
        git(self.remote, 'config', 'user.email', 'fixture@example.invalid')
        git(self.remote, 'add', '.')
        git(self.remote, 'commit', '-qm', 'initial')

    def command(self, *args):
        return subprocess.run([sys.executable, str(self.root / 'scripts/sdk.py'), *args], cwd=self.root,
                              text=True, capture_output=True, timeout=30)

    def commit(self):
        git(self.remote, 'add', '-A')
        git(self.remote, 'commit', '-qm', 'update')
        return git(self.remote, 'rev-parse', 'HEAD')

    def testReadOnlyCheckAndExplicitApplyAddDelete(self):
        (self.remote / 'README.md').unlink()
        (self.remote / 'new-note.md').write_text('Fixture SDK update\n')
        revision = self.commit()
        lockBefore = (self.root / 'sdk.lock.json').read_bytes()
        args = ['--source-repo', str(self.remote), '--ref', revision]
        result = self.command('check', *args)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('new-note.md', result.stdout)
        self.assertIn('README.md', result.stdout)
        self.assertEqual((self.root / 'sdk.lock.json').read_bytes(), lockBefore)
        self.assertTrue((self.root / 'vendor/FxPatchSDK/README.md').exists())
        result = self.command('update', *args)
        self.assertNotEqual(result.returncode, 0)
        result = self.command('update', *args, '--apply')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.root / 'vendor/FxPatchSDK/README.md').exists())
        self.assertTrue((self.root / 'vendor/FxPatchSDK/new-note.md').exists())
        self.assertEqual(json.loads((self.root / 'sdk.lock.json').read_text())['commit'], revision)
        self.assertEqual(self.command('verify').returncode, 0)

    def testModifiedSnapshotAndIncompleteImportArePreserved(self):
        revision = git(self.remote, 'rev-parse', 'HEAD')
        header = self.root / 'vendor/FxPatchSDK/source/Patch.h'
        header.write_text(header.read_text() + '\n// local edit\n')
        lockBefore = (self.root / 'sdk.lock.json').read_bytes()
        result = self.command('update', '--source-repo', str(self.remote), '--ref', revision, '--apply')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('local', result.stderr.lower())
        self.assertIn('local edit', header.read_text())
        self.assertEqual((self.root / 'sdk.lock.json').read_bytes(), lockBefore)
        header.write_bytes((ROOT / 'vendor/FxPatchSDK/source/Patch.h').read_bytes())
        (self.remote / 'internal/PatchABI.h').unlink()
        revision = self.commit()
        result = self.command('update', '--source-repo', str(self.remote), '--ref', revision, '--apply')
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue((self.root / 'vendor/FxPatchSDK/internal/PatchABI.h').exists())
        self.assertEqual((self.root / 'sdk.lock.json').read_bytes(), lockBefore)

    def testInterruptedReplacementRollsBack(self):
        sys.path.insert(0, str(ROOT / 'scripts'))
        from sdk import applySnapshot, fetchSnapshot, verifySnapshot
        from unittest.mock import patch
        revision = git(self.remote, 'rev-parse', 'HEAD')
        lockBefore = (self.root / 'sdk.lock.json').read_bytes()
        headerBefore = (self.root / 'vendor/FxPatchSDK/source/Patch.h').read_bytes()
        stage, _ = fetchSnapshot(self.root, revision, self.remote)
        original = Path.replace

        def interrupt(path, target):
            if path.name == 'nextLock.json':
                raise KeyboardInterrupt('fixture interruption')
            return original(path, target)

        with patch.object(Path, 'replace', interrupt), self.assertRaises(KeyboardInterrupt):
            applySnapshot(self.root, stage)
        self.assertEqual((self.root / 'sdk.lock.json').read_bytes(), lockBefore)
        self.assertEqual((self.root / 'vendor/FxPatchSDK/source/Patch.h').read_bytes(), headerBefore)
        self.assertEqual(verifySnapshot(self.root)['commit'], json.loads(lockBefore)['commit'])

    def testPendingJournalRecoversAfterProcessExit(self):
        sys.path.insert(0, str(ROOT / 'scripts'))
        from sdk import fetchSnapshot, verifySnapshot
        revision = git(self.remote, 'rev-parse', 'HEAD')
        stage, _ = fetchSnapshot(self.root, revision, self.remote)
        lockBefore = (self.root / 'sdk.lock.json').read_bytes()
        (stage / 'previousLock.json').write_bytes(lockBefore)
        journal = self.root / 'build/sdkUpdatePending.json'
        journal.write_text(json.dumps({'stage': str(stage.relative_to(self.root))}))
        (self.root / 'vendor/FxPatchSDK').replace(stage / 'previous')
        (stage / 'snapshot').replace(self.root / 'vendor/FxPatchSDK')
        (self.root / 'sdk.lock.json').write_text('{}')
        verifySnapshot(self.root)
        self.assertEqual((self.root / 'sdk.lock.json').read_bytes(), lockBefore)
        self.assertFalse(journal.exists())

    def testAbiChangeIsReportedAndInvalidRefLeavesState(self):
        abi = self.remote / 'internal/PatchABI.h'
        abi.write_text(abi.read_text().replace('0x000Bu', '0x000Cu'))
        revision = self.commit()
        result = self.command('check', '--source-repo', str(self.remote), '--ref', revision)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('ABI changed', result.stdout)
        result = self.command('update', '--source-repo', str(self.remote), '--ref', 'nonexistent', '--apply')
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.command('verify').returncode, 0)


if __name__ == '__main__':
    unittest.main()
