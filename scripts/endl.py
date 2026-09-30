#!/usr/bin/env python3
"""Validate the supported ARM32 PatchHeader against its pinned SDK contract."""
import argparse
import json
from pathlib import Path
import re
import struct
import sys

from project import ROOT
from sdk import verifySnapshot

ENTRY_NAMES = ['init', 'agent_update_buffers', 'agent_set_buffer', 'agent_get_buffer_size',
               'agent_get_param_min', 'agent_get_param_max', 'agent_get_param_default',
               'agent_is_param_enabled', 'agent_get_param_name', 'agent_get_param_unit',
               'agent_set_param', 'agent_special_action', 'agent_get_state_idx']
HEADER_FORMAT = '<IHH13I3I16I'
HEADER_FIELDS = ['magic', 'abi_version', 'flags', *ENTRY_NAMES, 'image_size', 'bss_begin', 'bss_size', 'reserved']


def sdkContract(root):
    verifySnapshot(root)
    sdk = root / 'vendor/FxPatchSDK'
    header = (sdk / 'internal/PatchABI.h').read_text()
    noComments = re.sub(r'//[^\n]*|/\*.*?\*/', '', header, flags=re.S)
    body = re.search(r'typedef struct PatchHeader\s*\{(.*?)\}\s*PatchHeader;', noComments, re.S)
    declarations = body[1].split(';') if body else []
    fields = [re.search(r'(\w+)\s*(?:\[16\])?\s*$', declaration.strip())[1]
              for declaration in declarations if declaration.strip()]
    if fields != HEADER_FIELDS or '#define PATCH_ABI_VERSION 0x000Bu' not in header:
        raise ValueError('Unsupported SDK header/ABI; review the artifact inspector before trusting new images')
    linker = (sdk / 'internal/patch_imx.ld').read_text()
    region = re.search(r'RAM\s*\(rxw\)\s*:\s*ORIGIN\s*=\s*_PATCH_BASE,\s*LENGTH\s*=\s*(\d+)(K?)', linker)
    if not region:
        raise ValueError('Unsupported SDK linker memory layout')
    return {'abiVersion': 11, 'ramBytes': int(region[1]) * (1024 if region[2] else 1)}


def inspectImage(path, root=ROOT, loadAddress=0x80000000):
    contract = sdkContract(root)
    data = Path(path).read_bytes()
    headerBytes = struct.calcsize(HEADER_FORMAT)
    if len(data) < headerBytes:
        raise ValueError('Truncated PatchHeader')
    values = struct.unpack_from(HEADER_FORMAT, data)
    magic, version, flags = values[:3]
    entries = dict(zip(ENTRY_NAMES, values[3:16]))
    imageSize, bssBegin, bssSize = values[16:19]
    if magic != 0x48435450 or version != contract['abiVersion'] or flags != 0 or any(values[19:]):
        raise ValueError('Invalid magic, ABI, flags or reserved fields')
    if headerBytes + imageSize != len(data):
        raise ValueError('Declared image size does not match file length')
    if not entries['init']:
        raise ValueError('Missing mandatory initialization entry')
    if loadAddress < 0 or loadAddress + contract['ramBytes'] > 2**32:
        raise ValueError('Invalid ARM32 load region')
    for name, pointer in entries.items():
        if pointer and (not pointer & 1 or not loadAddress + headerBytes <= (pointer & ~1) < loadAddress + len(data)):
            raise ValueError('Invalid Thumb entry pointer: ' + name)
    if len(data) > contract['ramBytes']:
        raise ValueError('Image exceeds linker RAM region')
    if bssSize or bssBegin:
        if bssBegin % 4 or bssBegin < loadAddress + len(data) or bssBegin + bssSize > loadAddress + contract['ramBytes']:
            raise ValueError('Invalid or overlapping BSS bounds')
    ramBytes = max(len(data), bssBegin + bssSize - loadAddress if bssBegin else 0)
    return {'artifact': str(path), 'abiVersion': version, 'headerBytes': headerBytes,
            'fileBytes': len(data), 'bssBytes': bssSize, 'ramBytes': ramBytes,
            'ramBudget': contract['ramBytes'], 'loadAddress': hex(loadAddress), 'entries': entries}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('--load-address', type=lambda value: int(value, 0), default=0x80000000)
    args = parser.parse_args()
    try:
        print(json.dumps(inspectImage(args.image, ROOT, args.load_address), indent=2))
        return 0
    except (ValueError, OSError, KeyError) as error:
        print('error: ' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
