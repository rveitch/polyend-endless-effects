#!/usr/bin/env python3
"""Check and explicitly update an immutable official SDK snapshot."""
import argparse
import io
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile

from project import ROOT, hashFile, run, safePath, writeJson

REQUIRED = ['LICENSE.TXT', 'Makefile', 'source/Patch.h', 'source/PatchImpl.cpp',
            'internal/PatchABI.h', 'internal/PatchCppWrapper.cpp', 'internal/PatchCppWrapper.h',
            'internal/patch_main.c', 'internal/patch_imx.ld']


def snapshotHashes(directory):
    files = {}
    for path in sorted(directory.rglob('*')):
        if path.is_symlink():
            raise ValueError('SDK snapshots cannot contain symlinks: ' + str(path))
        if path.is_file():
            files[str(path.relative_to(directory))] = hashFile(path)
    return files


def recoverPending(root):
    journal = root / 'build/sdkUpdatePending.json'
    if not journal.exists():
        return
    transaction = json.loads(journal.read_text())
    stage = safePath(root, transaction['stage'])
    vendor = root / 'vendor/FxPatchSDK'
    backup = stage / 'previous'
    if backup.exists():
        if vendor.exists():
            shutil.rmtree(vendor)
        backup.replace(vendor)
    (root / 'sdk.lock.json').write_bytes((stage / 'previousLock.json').read_bytes())
    journal.unlink()
    shutil.rmtree(stage)


def verifySnapshot(root=ROOT):
    recoverPending(root)
    lock = json.loads((root / 'sdk.lock.json').read_text())
    if lock.get('schemaVersion') != 1 or not re.fullmatch(r'[0-9a-f]{40}', lock.get('commit', '')):
        raise ValueError('Unsupported or invalid SDK lock')
    if snapshotHashes(root / 'vendor/FxPatchSDK') != lock['files']:
        raise ValueError('SDK snapshot has local modifications; preserve/review them before an update')
    return lock


def abiVersion(directory):
    text = (directory / 'internal/PatchABI.h').read_text()
    match = re.search(r'#define\s+PATCH_ABI_VERSION\s+(0x[0-9a-fA-F]+)', text)
    if not match:
        raise ValueError('SDK ABI version is missing')
    return int(match[1], 16)


def fetchSnapshot(root, ref, sourceRepo=None):
    lock = verifySnapshot(root)
    parent = root / 'build/sdkUpdates'
    parent.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix='update-', dir=parent))
    try:
        gitDir = stage / 'repository'
        gitDir.mkdir()
        run(['git', 'init', '-q', str(gitDir)], root, True)
        source = str(Path(sourceRepo).resolve()) if sourceRepo else lock['repository']
        run(['git', '-C', str(gitDir), 'fetch', '--depth=1', source, ref], root, True)
        commit = run(['git', '-C', str(gitDir), 'rev-parse', 'FETCH_HEAD^{commit}'], root, True).stdout.strip()
        archive = subprocess.run(['git', '-C', str(gitDir), 'archive', commit], check=True,
                                 capture_output=True, timeout=60).stdout
        destination = stage / 'snapshot'
        destination.mkdir()
        with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
            for member in stream.getmembers():
                if member.issym() or member.islnk() or not (member.isfile() or member.isdir()):
                    raise ValueError('Unsupported SDK archive member: ' + member.name)
                safePath(destination, member.name)
            stream.extractall(destination, filter='data')
        for relative in REQUIRED:
            if not (destination / relative).is_file():
                raise ValueError('Incomplete SDK import, missing ' + relative)
        newLock = {**lock, 'commit': commit, 'files': snapshotHashes(destination)}
        if sourceRepo:
            newLock['importSource'] = source
        else:
            newLock.pop('importSource', None)
        oldVersion = abiVersion(root / 'vendor/FxPatchSDK')
        newVersion = abiVersion(destination)
        paths = sorted(set(lock['files']) | set(newLock['files']))
        changes = [{'path': path, 'status': 'added' if path not in lock['files'] else
                    'deleted' if path not in newLock['files'] else 'modified'}
                   for path in paths if lock['files'].get(path) != newLock['files'].get(path)]
        report = {'oldCommit': lock['commit'], 'newCommit': commit, 'changes': changes,
                  'abiBefore': oldVersion, 'abiAfter': newVersion,
                  'buildRulesReview': any(c['path'] == 'Makefile' for c in changes)}
        if oldVersion != newVersion:
            report['warning'] = 'ABI changed: rebuild every effect and review firmware compatibility'
        writeJson(stage / 'nextLock.json', newLock)
        return stage, report
    except BaseException:
        shutil.rmtree(stage)
        raise


def applySnapshot(root, stage):
    verifySnapshot(root)
    vendor = root / 'vendor/FxPatchSDK'
    journal = root / 'build/sdkUpdatePending.json'
    (stage / 'previousLock.json').write_bytes((root / 'sdk.lock.json').read_bytes())
    writeJson(journal, {'stage': str(stage.relative_to(root))})
    try:
        vendor.replace(stage / 'previous')
        (stage / 'snapshot').replace(vendor)
        (stage / 'nextLock.json').replace(root / 'sdk.lock.json')
        # Journal removal is the transaction's commit point. A killed process
        # before this point is rolled back by the next SDK verify/build call.
        journal.unlink()
    except BaseException:
        recoverPending(root)
        raise
    shutil.rmtree(stage)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['verify', 'check', 'update'])
    parser.add_argument('--ref', default='master')
    parser.add_argument('--source-repo', help='Explicit local Git mirror for offline imports/testing')
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    try:
        if args.command == 'verify':
            print('SDK verified: ' + verifySnapshot()['commit'])
            return 0
        if args.command == 'update' and not args.apply:
            raise ValueError('SDK update requires an explicit --apply after reviewing check output')
        stage, report = fetchSnapshot(ROOT, args.ref, args.source_repo)
        print(json.dumps(report, indent=2))
        if args.command == 'update':
            applySnapshot(ROOT, stage)
            print('SDK imported. Run all host tests, ARM builds and migration comparisons before committing.')
        else:
            shutil.rmtree(stage)
        return 0
    except (ValueError, OSError, KeyError, subprocess.SubprocessError, tarfile.TarError) as error:
        print('error: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
