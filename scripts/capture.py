#!/usr/bin/env python3
"""Capture a selected effect as stereo float WAV with reproducible provenance."""
import argparse
from array import array
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from audio import makeStimulus, readWav, writeWav
from project import ROOT, gitIdentity, hashFile, loadEffect, run, sourceHashes, writeJson
from sdk import verifySnapshot


def validateEvents(events, frames):
    if not isinstance(events, list):
        raise ValueError('Events must be a JSON list')
    for event in events:
        if not isinstance(event, dict) or type(event.get('frame')) is not int or not 0 <= event['frame'] < frames:
            raise ValueError('Event frame must be inside the capture')
        if 'action' in event and 'parameter' not in event and type(event['action']) is int and event['action'] in (0, 1):
            continue
        value = event.get('value')
        if ('parameter' not in event or 'action' in event or type(event['parameter']) is not int or
                event['parameter'] not in (0, 1, 2) or type(value) not in (int, float) or
                not math.isfinite(value) or not 0 <= value <= 1):
            raise ValueError('Invalid normalized parameter event')
    return sorted(events, key=lambda event: event['frame'])


def renderRaw(source, samples, parameters, events, blockSize, output, hostCxx='c++'):
    if sys.byteorder != 'little':
        raise ValueError('Host capture harness currently requires little-endian float32')
    with tempfile.TemporaryDirectory(prefix='endless-capture-') as temporary:
        folder = Path(temporary)
        binary = folder / 'probe'
        flags = ['-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror', '-fno-exceptions', '-fno-rtti']
        compiler = run([hostCxx, '--version'], capture=True).stdout.splitlines()[0]
        run([hostCxx, *flags, '-I', str(ROOT / 'vendor/FxPatchSDK/source'),
             str(ROOT / 'tests/captureProbe.cpp'), str(source), '-o', str(binary)])
        inputPath = folder / 'input.f32'
        inputPath.write_bytes(samples.tobytes())
        eventPath = folder / 'events.txt'
        eventPath.write_text(''.join(f"{e['frame']} {'action' if 'action' in e else 'parameter'} "
                                     f"{e.get('action', e.get('parameter'))} {e.get('value', 0)}\n" for e in events))
        raw = folder / 'output.f32'
        result = run([str(binary), str(inputPath), str(raw), str(blockSize), str(eventPath),
                      *['default' if value is None else repr(value) for value in parameters]], capture=True)
        details = json.loads(result.stdout)
        processed = array('f')
        processed.frombytes(raw.read_bytes())
        writeWav(output, processed)
        return {**details, 'compiler': compiler, 'flags': flags, 'probeHash': hashFile(ROOT / 'tests/captureProbe.cpp')}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--effect', default='junoChorus')
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--input', type=Path)
    parser.add_argument('--signal', choices=['sine', 'impulse', 'broadband'], default='sine')
    parser.add_argument('--seconds', type=float)
    parser.add_argument('--scale', type=float, default=0.2)
    parser.add_argument('--frequency', type=float, default=220.0)
    parser.add_argument('--seed', type=int, default=1)
    parser.add_argument('--stereo', choices=['distinct', 'mono'], default='distinct')
    parser.add_argument('--events', type=Path)
    parser.add_argument('--block-size', type=int, default=128)
    parser.add_argument('--host-cxx', default=os.environ.get('HOST_CXX', 'c++'))
    for index in range(3):
        parser.add_argument('--param' + str(index), type=float)
    args = parser.parse_args()
    try:
        effect = loadEffect(ROOT, args.effect)
        lock = verifySnapshot()
        parameters = [args.param0, args.param1, args.param2]
        if any(value is not None and (not math.isfinite(value) or not 0 <= value <= 1) for value in parameters):
            raise ValueError('Knob values must be finite normalized 0..1 values')
        if not 1 <= args.block_size <= 65536:
            raise ValueError('Callback size must be 1..65536 samples')
        output = args.output.resolve()
        sidecar = output.with_suffix('.json')
        inputCopy = output.with_name(output.stem + '.input.wav')
        if output.suffix.lower() != '.wav' or any(p.exists() for p in [output, sidecar, inputCopy]):
            raise ValueError('Use a new .wav output name; captures and sidecars are never overwritten')
        if args.input:
            if args.seconds is not None:
                raise ValueError('Input WAV captures use the full file; omit --seconds')
            samples, rate, channels = readWav(args.input)
            if rate != 48000:
                raise ValueError('Input must be 48000 Hz; resampling is not implicit')
            if channels == 1:
                samples = array('f', (v for value in samples for v in (value, value)))
            stimulus = {'kind': 'wav', 'path': str(args.input.resolve()), 'fileHash': hashFile(args.input), 'originalChannels': channels}
        else:
            seconds = args.seconds if args.seconds is not None else 12.0
            if not math.isfinite(seconds) or not 0 < seconds <= 600 or not math.isfinite(args.frequency) or not 0 < args.frequency < 24000:
                raise ValueError('Duration must be finite in (0,600] seconds and frequency inside Nyquist')
            samples = makeStimulus(args.signal, round(seconds * 48000), args.scale, args.seed, args.frequency, args.stereo)
            stimulus = {'kind': args.signal, 'scale': args.scale, 'seed': args.seed, 'frequency': args.frequency, 'stereo': args.stereo}
        frames = len(samples) // 2
        if frames < 1:
            raise ValueError('Empty capture input')
        events = validateEvents(json.loads(args.events.read_text()) if args.events else [], frames)
        details = renderRaw(ROOT / effect['source'], samples, parameters, events, args.block_size, output, args.host_cxx)
        writeWav(inputCopy, samples)
        writeJson(sidecar, {'schemaVersion': 1, 'effectId': args.effect, 'sampleRate': 48000, 'channels': 2,
                           'frames': frames, 'durationSeconds': frames / 48000, 'callbackSize': args.block_size,
                           'parameters': details['parameters'], 'events': events, 'stimulus': stimulus,
                           'inputCopy': str(inputCopy), 'inputHash': hashFile(inputCopy), 'captureHash': hashFile(output),
                           'sdkCommit': lock['commit'], 'sourceHashes': sourceHashes(ROOT, effect),
                           'repository': gitIdentity(), 'hostBuild': details})
        print('Captured ' + str(output) + ' (' + str(frames) + ' stereo frames)')
        return 0
    except (ValueError, OSError, KeyError, subprocess.SubprocessError) as error:
        print('error: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
