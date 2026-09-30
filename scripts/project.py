"""Shared catalog, provenance and finite-process helpers."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def hashFile(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def writeJson(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2, allow_nan=False) + '\n')
    temporary.replace(path)


def run(command, root=ROOT, capture=False):
    return subprocess.run([str(part) for part in command], cwd=root, check=True,
                          capture_output=capture, text=True, timeout=180)


def safePath(root, relative):
    if not isinstance(relative, str) or Path(relative).is_absolute() or '..' in Path(relative).parts:
        raise ValueError('Expected a repository-relative path without parent traversal')
    path = (root / relative).resolve()
    if not path.is_relative_to(root.resolve()):
        raise ValueError('Path escapes repository: ' + relative)
    return path


def loadCatalog(root=ROOT):
    catalog = json.loads((root / 'effects/catalog.json').read_text())
    if catalog.get('schemaVersion') != 1 or not isinstance(catalog.get('effects'), list):
        raise ValueError('Unsupported effect catalog')
    entries = {}
    for effect in catalog['effects']:
        effectId = effect['id']
        if not re.fullmatch(r'[A-Za-z][A-Za-z0-9]*', effectId) or effectId in entries or effectId == 'all':
            raise ValueError('Invalid or duplicate effect ID: ' + str(effectId))
        for relative in [effect['source'], effect['documentation'], *(t['source'] for t in effect['tests'])]:
            if not safePath(root, relative).is_file():
                raise ValueError('Missing catalog file: ' + relative)
        entries[effectId] = effect
    if not entries:
        raise ValueError('Empty effect catalog')
    return entries


def loadEffect(root, effectId):
    entries = loadCatalog(root)
    if effectId not in entries:
        raise ValueError('Unknown effect: ' + effectId)
    return entries[effectId]


def sourceHashes(root, effect):
    files = set((root / 'vendor/FxPatchSDK').rglob('*'))
    files.update((root / effect['source']).parent.rglob('*'))
    return {str(p.relative_to(root)): hashFile(p) for p in sorted(files)
            if p.is_file() and p.suffix in {'.cpp', '.c', '.h', '.hpp', '.ld'}}


def gitIdentity(root=ROOT):
    try:
        revision = run(['git', 'rev-parse', 'HEAD'], root, True).stdout.strip()
        dirty = bool(run(['git', 'status', '--porcelain'], root, True).stdout.strip())
        return {'revision': revision, 'dirty': dirty}
    except subprocess.CalledProcessError:
        return {'revision': None, 'dirty': True}
