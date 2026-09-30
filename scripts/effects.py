#!/usr/bin/env python3
"""List, build and test independently selected effects."""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from project import ROOT, dependencyHashes, gitIdentity, hashFile, loadCatalog, run, writeJson


def verifySdk():
    from sdk import verifySnapshot
    return verifySnapshot(ROOT)


def buildEffect(effect, args):
    lock = verifySdk()
    if not re.fullmatch(r'[A-Za-z0-9_-]+', args.patch_name or effect['id']):
        raise ValueError('Invalid patch name')
    name = args.patch_name or effect['id']
    output = Path(args.build_dir).resolve() if args.build_dir else ROOT / 'build' / effect['id']
    output.mkdir(parents=True, exist_ok=True)
    compiler = run([args.toolchain + 'g++', '--version'], capture=True).stdout.splitlines()[0]
    configPath = output / 'objects' / effect['id'] / 'buildConfig.json'
    config = {'toolchain': args.toolchain, 'compiler': compiler, 'extraFlags': args.extra_flags,
              'loadAddress': args.load_address, 'makefileHash': hashFile(ROOT / 'buildSupport/arm.mk'),
              'sdkCommit': lock['commit'], 'source': effect['source']}
    timestamp = datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f')
    artifact = output / (name + '_' + timestamp + '.endl')
    command = ['make', '-f', 'buildSupport/arm.mk', 'TOOLCHAIN=' + args.toolchain,
               'EFFECT_ID=' + effect['id'], 'PATCH_IMPL=' + effect['source'], 'BUILD_DIR=' + str(output),
               'PATCH_NAME=' + name, 'PATCH_BIN=' + str(artifact), 'PATCH_LOAD_ADDR=' + args.load_address,
               'CUSTOM_COMPILER_OPTIONS=' + args.extra_flags]
    if not configPath.exists():
        writeJson(configPath, {})
    # Remove prior scans so changing the catalog source cannot retain obsolete dependencies.
    for dependencyFile in configPath.parent.rglob('*.d'):
        dependencyFile.unlink()
    run([*command, 'dependencyScan'])
    inputs = dependencyHashes(ROOT, configPath.parent.rglob('*.d'))
    inputs['vendor/FxPatchSDK/internal/patch_imx.ld'] = hashFile(ROOT / 'vendor/FxPatchSDK/internal/patch_imx.ld')
    config['inputHashes'] = inputs
    if json.loads(configPath.read_text()) != config:
        # Content identity also handles same-timestamp changes on older Make.
        shutil.rmtree(configPath.parent)
        writeJson(configPath, config)
    run(command)
    from endl import inspectImage
    inspection = inspectImage(artifact, ROOT, int(args.load_address, 0))
    elf = artifact.with_suffix('.elf')
    shutil.copy2(output / (name + '.elf'), elf)
    flags = run(['make', '-f', 'buildSupport/arm.mk', '-np', *command[3:]], capture=True).stdout
    flagLines = [line for line in flags.splitlines() if line.startswith(('CFLAGS :=', 'CXXFLAGS :=', 'LDFLAGS :='))]
    manifest = {'schemaVersion': 1, 'effectId': effect['id'], 'sdkCommit': lock['commit'],
                'inspection': inspection, 'compiler': compiler, 'flags': flagLines, 'configuration': config,
                'repository': gitIdentity(), 'sourceHashes': inputs,
                'artifact': str(artifact), 'artifactHash': hashFile(artifact),
                'elf': str(elf), 'elfHash': hashFile(elf)}
    writeJson(artifact.with_suffix('.manifest.json'), manifest)
    print('Built ' + str(artifact))
    return artifact


def testEffect(effect, args):
    output = Path(args.build_dir).resolve() if args.build_dir else ROOT / 'build' / effect['id'] / 'host'
    output.mkdir(parents=True, exist_ok=True)
    for test in effect['tests']:
        binary = output / Path(test['source']).stem
        command = [args.host_cxx, '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-I', str(ROOT / 'vendor/FxPatchSDK/source'), test['source']]
        if test.get('linkEffect'):
            command.append(effect['source'])
        if args.sanitize:
            command.append('-fsanitize=undefined')
        run([*command, '-o', str(binary)])
        run([str(binary)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['list', 'build', 'test', 'check', 'clean'])
    parser.add_argument('--effect', default='junoChorus')
    parser.add_argument('--build-dir')
    parser.add_argument('--patch-name')
    parser.add_argument('--toolchain', default=os.environ.get('TOOLCHAIN', 'arm-none-eabi-'))
    parser.add_argument('--host-cxx', default=os.environ.get('HOST_CXX', 'c++'))
    parser.add_argument('--extra-flags', default='')
    parser.add_argument('--load-address', default='0x80000000')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    try:
        catalog = loadCatalog()
        if args.command == 'list':
            for effect in catalog.values():
                print(effect['id'] + ': ' + effect['name'])
            return 0
        if args.effect != 'all' and args.effect not in catalog:
            raise ValueError('Unknown effect: ' + args.effect)
        selected = list(catalog.values()) if args.effect == 'all' else [catalog[args.effect]]
        if len(selected) > 1 and (args.build_dir or args.patch_name):
            raise ValueError('Use per-effect default directories/names for --effect all')
        for effect in selected:
            if args.command == 'build':
                buildEffect(effect, args)
            elif args.command == 'test':
                testEffect(effect, args)
            elif args.command == 'check':
                from endl import inspectImage
                directory = Path(args.build_dir).resolve() if args.build_dir else ROOT / 'build' / effect['id']
                images = sorted(directory.glob('*.endl'))
                if not images:
                    raise ValueError('No artifacts for effect: ' + effect['id'])
                for image in images:
                    print(json.dumps(inspectImage(image, ROOT, int(args.load_address, 0))))
            else:
                directory = Path(args.build_dir).resolve() if args.build_dir else ROOT / 'build' / effect['id']
                managed = (ROOT / 'build' / effect['id']).resolve()
                if directory != managed:
                    raise ValueError('Clean only removes the default managed per-effect build directory')
                if directory.exists():
                    shutil.rmtree(directory)
        return 0
    except (ValueError, OSError, KeyError, subprocess.SubprocessError) as error:
        print('error: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
