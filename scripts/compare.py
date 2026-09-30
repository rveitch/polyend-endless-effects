#!/usr/bin/env python3
"""Compare raw stereo captures without changing gain or timing."""
import argparse
from array import array
import cmath
import json
import math
from pathlib import Path
import sys

from audio import readWav, writeWav
from capture import validateEvents
from project import hashFile, writeJson


def metrics(values):
    rms = math.sqrt(math.fsum(value * value for value in values) / len(values))
    peak = max(abs(value) for value in values)
    return {'finiteSamples': len(values), 'clippedSamples': sum(abs(value) >= 1 for value in values),
            'rms': rms, 'peak': peak, 'rmsDbfs': 20 * math.log10(rms) if rms else None,
            'peakDbfs': 20 * math.log10(peak) if peak else None}


def correlation(a, b, centered=True):
    meanA = math.fsum(a) / len(a) if centered else 0.0
    meanB = math.fsum(b) / len(b) if centered else 0.0
    covariance = math.fsum((x - meanA) * (y - meanB) for x, y in zip(a, b))
    denominator = math.sqrt(math.fsum((x - meanA) ** 2 for x in a) * math.fsum((y - meanB) ** 2 for y in b))
    return max(-1.0, min(1.0, covariance / denominator)) if denominator else None


def spectrum(values, size):
    data = [complex(values[index] * (0.5 - 0.5 * math.cos(2 * math.pi * index / (size - 1)))) for index in range(size)]
    reverse = 0
    for index in range(1, size):
        bit = size // 2
        while reverse & bit:
            reverse ^= bit
            bit //= 2
        reverse ^= bit
        if index < reverse:
            data[index], data[reverse] = data[reverse], data[index]
    length = 2
    while length <= size:
        rotation = cmath.exp(-2j * math.pi / length)
        for start in range(0, size, length):
            factor = 1 + 0j
            for offset in range(length // 2):
                even = data[start + offset]
                odd = factor * data[start + offset + length // 2]
                data[start + offset] = even + odd
                data[start + offset + length // 2] = even - odd
                factor *= rotation
        length *= 2
    return [abs(value) for value in data[:size // 2 + 1]]


def compareSamples(a, b, sampleRate):
    if not a or len(a) != len(b) or len(a) % 2 or sampleRate <= 0:
        raise ValueError('Captures must have equal nonempty stereo dimensions')
    if not all(math.isfinite(value) for samples in (a, b) for value in samples):
        raise ValueError('Captures contain nonfinite samples')
    channels = []
    for samples in (a, b):
        left = list(samples[0::2])
        right = list(samples[1::2])
        channels.append({'left': left, 'right': right, 'mono': [(x + y) / 2 for x, y in zip(left, right)]})
    frames = len(a) // 2
    fftSize = 2 ** min(15, frames.bit_length() - 1)
    report = {'sampleRate': sampleRate, 'frames': frames, 'monoConvention': '(L + R) / 2', 'durationSeconds': frames / sampleRate,
              'exactSamples': a == b, 'a': {}, 'b': {}, 'difference': {},
              'spectrum': {'window': 'Hann', 'startFrame': 0, 'frames': fftSize,
                           'binHz': sampleRate / fftSize, 'channels': {}}}
    for label, channel in zip(('a', 'b'), channels):
        report[label] = {name: metrics(values) for name, values in channel.items()}
        report[label]['stereoCorrelation'] = correlation(channel['left'], channel['right'], centered=False)
    for name in ('left', 'right', 'mono'):
        x, y = channels[0][name], channels[1][name]
        difference = metrics([second - first for first, second in zip(x, y)])
        rmsA, rmsB = report['a'][name]['rms'], report['b'][name]['rms']
        difference['gainDb'] = 20 * math.log10(rmsB / rmsA) if rmsA and rmsB else None
        difference['correlation'] = correlation(x, y)
        report['difference'][name] = difference
        if fftSize >= 4:
            first, second = spectrum(x, fftSize), spectrum(y, fftSize)
            report['spectrum']['channels'][name] = {'magnitudeCorrelation': correlation(first, second),
                'magnitudeDifferenceRms': metrics([v - u for u, v in zip(first, second)])['rms']}
    return report


def captureInfo(path, sampleRate, channels, frames):
    sidecar = path.with_suffix('.json')
    if not sidecar.exists():
        return {'path': str(path), 'hash': hashFile(path), 'metadata': None}
    metadata = json.loads(sidecar.read_text())
    if (not isinstance(metadata, dict) or metadata.get('sampleRate') != sampleRate or
            metadata.get('channels') != channels or metadata.get('frames') != frames or
            metadata.get('captureHash', hashFile(path)) != hashFile(path)):
        raise ValueError('Capture metadata does not match WAV: ' + str(sidecar))
    if 'schemaVersion' in metadata and (type(metadata['schemaVersion']) is not int or metadata['schemaVersion'] != 1):
        raise ValueError('Unsupported capture metadata schema: ' + str(sidecar))
    for field in ('sampleRate', 'channels', 'frames'):
        if type(metadata[field]) is not int:
            raise ValueError('Invalid integer capture metadata: ' + field)
    if 'durationSeconds' in metadata:
        duration = metadata['durationSeconds']
        if (type(duration) not in (int, float) or not math.isfinite(duration) or
                not math.isclose(duration, frames / sampleRate, rel_tol=1e-9, abs_tol=1e-9)):
            raise ValueError('Capture metadata duration contradicts WAV')
    if 'events' in metadata:
        validateEvents(metadata['events'], frames)
    if 'callbackSize' in metadata and (type(metadata['callbackSize']) is not int or not 1 <= metadata['callbackSize'] <= 65536):
        raise ValueError('Invalid callback size in capture metadata')
    if 'parameters' in metadata:
        parameters = metadata['parameters']
        if (not isinstance(parameters, list) or len(parameters) != 3 or
                any(type(value) not in (int, float) or not math.isfinite(value) or not 0 <= value <= 1 for value in parameters)):
            raise ValueError('Invalid normalized parameters in capture metadata')
    return {'path': str(path), 'hash': hashFile(path), 'metadata': metadata}


def listeningCopy(samples, shift, targetDbfs):
    frames = len(samples) // 2
    if abs(shift) >= frames:
        raise ValueError('Listening shift must be smaller than capture length')
    shifted = ([0.0] * (shift * 2) + list(samples[:(frames - shift) * 2])) if shift >= 0 else (list(samples[-shift * 2:]) + [0.0] * (-shift * 2))
    rms = metrics(shifted)['rms']
    gain = 10 ** (targetDbfs / 20) / rms if rms else 1.0
    output = array('f', (value * gain for value in shifted))
    if max(abs(value) for value in output) > 1:
        raise ValueError('Requested listening gain would clip; choose a lower target')
    return output, {'shiftFrames': shift, 'gain': gain, 'targetRmsDbfs': targetDbfs}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('a', type=Path)
    parser.add_argument('b', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--preview-dir', type=Path)
    parser.add_argument('--target-dbfs', type=float, default=-20)
    parser.add_argument('--shift-a', type=int, default=0)
    parser.add_argument('--shift-b', type=int, default=0)
    args = parser.parse_args()
    try:
        a, rateA, channelsA = readWav(args.a)
        b, rateB, channelsB = readWav(args.b)
        if channelsA != 2 or channelsB != 2 or rateA != rateB:
            raise ValueError('Compare requires stereo WAVs at the same sample rate; no implicit resampling')
        previewPaths = [args.preview_dir / name for name in ('a.listening.wav', 'b.listening.wav', 'transforms.json')] if args.preview_dir else []
        destinations = [path.resolve() for path in previewPaths + ([args.output] if args.output else [])]
        if len(destinations) != len(set(destinations)):
            raise ValueError('Comparison destinations collide')
        if any(path.exists() for path in destinations):
            raise ValueError('Comparison output already exists')
        provenance = [captureInfo(path.resolve(), rate, channels, len(samples) // 2)
                      for path, samples, rate, channels in [(args.a, a, rateA, channelsA), (args.b, b, rateB, channelsB)]]
        report = compareSamples(a, b, rateA)
        report['captures'] = provenance
        if args.preview_dir:
            if not math.isfinite(args.target_dbfs) or not -120 <= args.target_dbfs <= 0:
                raise ValueError('Listening target must be finite in -120..0 dBFS')
            paths = previewPaths
            outputs = [listeningCopy(samples, shift, args.target_dbfs) for samples, shift in [(a, args.shift_a), (b, args.shift_b)]]
            for path, (samples, _) in zip(paths, outputs):
                writeWav(path, samples, rateA)
            writeJson(paths[2], {'captures': provenance, 'transformations': [details for _, details in outputs]})
            report['listeningCopies'] = [str(path.resolve()) for path in paths]
        elif args.shift_a or args.shift_b:
            raise ValueError('Shifts require --preview-dir; raw measurements are never shifted')
        if args.output:
            writeJson(args.output, report)
        print(json.dumps(report, indent=2, allow_nan=False))
        return 0
    except (ValueError, OSError, OverflowError) as error:
        print('error: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
