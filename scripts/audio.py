"""Minimal lossless float-WAV I/O and deterministic host stimuli."""
from array import array
import math
from pathlib import Path
import struct
import sys


def readWav(path):
    data = Path(path).read_bytes()
    if len(data) < 12 or data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        raise ValueError('Expected a little-endian RIFF/WAVE file')
    if struct.unpack_from('<I', data, 4)[0] + 8 != len(data):
        raise ValueError('Truncated or inconsistent RIFF length')
    offset = 12
    fmt = payload = None
    while offset < len(data):
        if offset + 8 > len(data):
            raise ValueError('Truncated WAV chunk header')
        name, size = struct.unpack_from('<4sI', data, offset)
        offset += 8
        chunk = data[offset:offset + size]
        if len(chunk) != size:
            raise ValueError('Truncated WAV chunk')
        if name == b'fmt ':
            if fmt is not None or size < 16:
                raise ValueError('Invalid or duplicate WAV format chunk')
            fmt = struct.unpack_from('<HHIIHH', chunk)
        elif name == b'data':
            if payload is not None:
                raise ValueError('Duplicate WAV audio chunk')
            payload = chunk
        offset += size + size % 2
    if fmt is None or payload is None:
        raise ValueError('WAV format/audio chunk missing')
    encoding, channels, rate, byteRate, blockAlign, bits = fmt
    if channels not in (1, 2) or rate <= 0 or bits % 8 or blockAlign != channels * bits // 8:
        raise ValueError('Unsupported WAV channels or block alignment')
    if bits not in (16, 24, 32) or not blockAlign:
        raise ValueError('Unsupported WAV sample width')
    if offset != len(data):
        raise ValueError('Missing WAV chunk padding')
    if byteRate != rate * blockAlign or len(payload) % blockAlign:
        raise ValueError('Inconsistent WAV byte rate or sample length')
    samples = array('f')
    if encoding == 3 and bits == 32:
        samples.frombytes(payload)
        if sys.byteorder != 'little':
            samples.byteswap()
    elif encoding == 1 and bits in (16, 24, 32):
        width = bits // 8
        scale = float(2 ** (bits - 1))
        samples.extend(int.from_bytes(payload[i:i + width], 'little', signed=True) / scale
                       for i in range(0, len(payload), width))
    else:
        raise ValueError('Supported WAV encodings: float32 and PCM16/24/32')
    if not all(math.isfinite(value) for value in samples):
        raise ValueError('Nonfinite WAV samples')
    return samples, rate, channels


def writeWav(path, samples, sampleRate=48000, channels=2):
    if channels not in (1, 2) or sampleRate <= 0 or len(samples) % channels:
        raise ValueError('Invalid WAV sample dimensions')
    floats = array('f', samples)
    if not all(math.isfinite(value) for value in floats):
        raise ValueError('Nonfinite WAV output')
    if sys.byteorder != 'little':
        floats.byteswap()
    payload = floats.tobytes()
    fmt = struct.pack('<4sIHHIIHH', b'fmt ', 16, 3, channels, sampleRate,
                      sampleRate * channels * 4, channels * 4, 32)
    fact = struct.pack('<4sII', b'fact', 4, len(samples) // channels)
    body = fmt + fact + struct.pack('<4sI', b'data', len(payload)) + payload
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    with Path(path).open('xb') as output:
        output.write(struct.pack('<4sI4s', b'RIFF', len(body) + 4, b'WAVE') + body)


def makeStimulus(kind, frames, scale=0.2, seed=1, frequency=220.0, stereo='distinct'):
    if frames < 1 or not math.isfinite(scale) or not 0 <= scale <= 1:
        raise ValueError('Invalid stimulus duration or scale')
    state = seed & 0xFFFFFFFF
    rightState = (seed ^ 0x9E3779B9) & 0xFFFFFFFF
    samples = array('f')
    for frame in range(frames):
        if kind == 'broadband':
            state = (1664525 * state + 1013904223) & 0xFFFFFFFF
            rightState = (1664525 * rightState + 1013904223) & 0xFFFFFFFF
            left = scale * (state / 2147483648.0 - 1.0)
            right = scale * (rightState / 2147483648.0 - 1.0)
        elif kind == 'sine':
            phase = 2 * math.pi * frequency * frame / 48000
            left = scale * math.sin(phase)
            right = scale * math.cos(phase)
        elif kind == 'impulse':
            left = scale if frame == 0 else 0.0
            right = -scale * 0.75 if frame == 0 else 0.0
        else:
            raise ValueError('Unknown stimulus: ' + kind)
        samples.extend((left, left if stereo == 'mono' else right))
    return samples
