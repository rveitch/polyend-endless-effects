#!/usr/bin/env python3
"""Verify a DSP relocation against a saved original source/header baseline."""
import argparse
from datetime import datetime, timezone
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from audio import makeStimulus, readWav
from capture import renderRaw
from project import hashFile, writeJson


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, default=ROOT / 'build/migration-baseline/PatchImpl.cpp')
    parser.add_argument('--seconds', type=float, default=12)
    args = parser.parse_args()
    if not args.baseline.is_file() or not 0 < args.seconds <= 600:
        parser.error('Provide an original PatchImpl.cpp with its original adjacent Patch.h and a finite duration')
    output = ROOT / 'build/migration-baseline' / datetime.now(timezone.utc).strftime('renders-%Y%m%dT%H%M%S%f')
    output.mkdir(parents=True)
    samples = makeStimulus('broadband', round(args.seconds * 48000), seed=42)
    scenarios = {'modeI': [], 'modeII': [{'frame': 0, 'action': 0}], 'modeIplusII': [{'frame': 0, 'action': 1}],
                 'transitions': [{'frame': 1501, 'action': 0}, {'frame': 1523, 'action': 1},
                                 {'frame': 1607, 'parameter': 1, 'value': 0.9}, {'frame': 3017, 'action': 0}]}
    results = []
    for name, events in scenarios.items():
        paths = [output / (name + suffix + '.wav') for suffix in ('-original64', '-current64', '-current127')]
        builds = []
        for source, block, path in zip([args.baseline, ROOT / 'effects/junoChorus/PatchImpl.cpp', ROOT / 'effects/junoChorus/PatchImpl.cpp'], [64, 64, 127], paths):
            builds.append(renderRaw(source, samples, [1.0, 0.5, 0.5], events, block, path))
        reference = readWav(paths[0])[0].tobytes()
        exact = all(readWav(path)[0].tobytes() == reference for path in paths[1:])
        results.append({'scenario': name, 'frames': len(samples) // 2, 'events': events, 'exact': exact,
                        'files': {str(path): hashFile(path) for path in paths}, 'hostBuilds': builds})
        print(name + ': ' + ('bit-identical' if exact else 'DIFFERENT'), flush=True)
    writeJson(output / 'results.json', {'baselineHash': hashFile(args.baseline), 'currentHash': hashFile(ROOT / 'effects/junoChorus/PatchImpl.cpp'), 'results': results})
    print('Evidence: ' + str(output / 'results.json'))
    return 0 if all(result['exact'] for result in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
