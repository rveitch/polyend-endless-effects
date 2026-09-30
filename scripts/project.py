"""Shared catalog, provenance and finite-process helpers."""
import hashlib
import json
from pathlib import Path
import re
import shlex
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


def gitIdentity(root=ROOT):
    try:
        revision = run(['git', 'rev-parse', 'HEAD'], root, True).stdout.strip()
        dirty = bool(run(['git', 'status', '--porcelain'], root, True).stdout.strip())
        return {'revision': revision, 'dirty': dirty}
    except subprocess.CalledProcessError:
        return {'revision': None, 'dirty': True}


def dependencyHashes(root, dependencyFiles):
    """Read compiler-generated Make dependencies, including shared/data includes."""
    files = set()
    for dependencyFile in dependencyFiles:
        text = dependencyFile.read_text().replace(chr(92) + chr(10), ' ')
        declaration = text.splitlines()[0]
        if ':' not in declaration:
            raise ValueError('Invalid compiler dependency file: ' + str(dependencyFile))
        for relative in shlex.split(declaration.split(':', 1)[1]):
            path = Path(relative.replace('$$', '$'))
            files.add(path.resolve() if path.is_absolute() else (root / path).resolve())
    if not files:
        raise ValueError('Compiler produced no source dependency identities')
    return {str(path.relative_to(root)) if path.is_relative_to(root) else str(path): hashFile(path)
            for path in sorted(files)}
