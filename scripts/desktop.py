#!/usr/bin/env python3
"""Build and test a catalog-selected desktop VST3 using pinned local JUCE."""
import argparse
import json
from pathlib import Path
import platform
import re
import subprocess
import sys
from project import ROOT, dependencyHashes, hashFile, gitIdentity, loadEffect, writeJson
from sdk import verifySnapshot


def runDesktop(command, capture=False):
    return subprocess.run(command, cwd=ROOT, check=True, text=True, capture_output=capture, timeout=600)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['build', 'test'])
    parser.add_argument('--effect', default='junoChorus')
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args()
    try:
        effect = loadEffect(ROOT, args.effect)
        metadata = effect.get('desktop', {})
        if not re.fullmatch('[A-Za-z]{4}', metadata.get('pluginCode', '')) or not re.fullmatch('[A-Za-z][A-Za-z0-9_]*', metadata.get('productName', '')):
            raise ValueError('Effect needs explicit desktop identity metadata')
        names = metadata.get('parameterNames')
        if not isinstance(names, list) or len(names) != 3 or any(not re.fullmatch('[A-Za-z][A-Za-z0-9 ]*', name) for name in names):
            raise ValueError('Invalid desktop parameter names')
        if not 1 <= args.jobs <= 32:
            raise ValueError('Build jobs must be 1..32')
        sdk = verifySnapshot()
        lock = json.loads((ROOT / 'desktop.lock.json').read_text())
        source = ROOT / lock['juce']['localSource']
        actual = runDesktop(['git', '-C', str(source), 'rev-parse', 'HEAD'], True).stdout.strip()
        dirty = runDesktop(['git', '-C', str(source), 'status', '--porcelain'], True).stdout.strip()
        if actual != lock['juce']['commit'] or dirty:
            raise ValueError('JUCE must match the clean pinned checkout')
        output = ROOT / 'build/desktop' / args.effect
        command = ['cmake', '-S', str(ROOT / 'desktop'), '-B', str(output), '-DEFFECT=' + args.effect, '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON']
        if platform.system() == 'Darwin':
            command.append('-DCMAKE_OSX_ARCHITECTURES=' + platform.machine())
        runDesktop(command)
        runDesktop(['cmake', '--build', str(output), '--parallel', str(args.jobs)])
        if args.command == 'test':
            if args.effect != 'junoChorus':
                raise ValueError('Processor regression suite currently targets junoChorus')
            runDesktop(['ctest', '--test-dir', str(output), '--output-on-failure'])
        bundles = list((output / 'EndlessPlugin_artefacts/Release/VST3').glob('*.vst3'))
        if len(bundles) != 1:
            raise ValueError('Expected exactly one built VST3 bundle')
        bundle = bundles[0]
        writeJson(output / 'desktop.manifest.json', {'schemaVersion': 1, 'effectId': args.effect, 'sdkCommit': sdk['commit'],
            'juceCommit': actual, 'repository': gitIdentity(), 'architecture': platform.machine(),
            'cmake': runDesktop(['cmake', '--version'], True).stdout.splitlines()[0], 'artifact': str(bundle),
            'configureCommand': command, 'sourceHashes': dependencyHashes(ROOT, output.rglob('*.o.d')),
            'compileCommandsHash': hashFile(output / 'compile_commands.json'),
            'artifactHashes': {str(path.relative_to(bundle)): hashFile(path) for path in sorted(bundle.rglob('*')) if path.is_file()}})
        print('Built ' + str(bundle))
        return 0
    except (ValueError, KeyError, OSError, subprocess.SubprocessError) as error:
        print('error: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
