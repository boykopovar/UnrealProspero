from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor
import gzip
import hashlib
import os
from pathlib import Path
import re
from urllib.request import Request, urlopen
from uuid import uuid4
from xml.etree import ElementTree

from .engine import contained_path


class Dependencies:
    def __init__(self, engine, cache):
        self.engine = Path(engine).resolve(strict=True)
        self.cache = Path(cache).resolve()
        self.cache.mkdir(parents=True, exist_ok=True)
        manifest = ElementTree.parse(self.engine / 'Engine/Build/Commit.gitdeps.xml').getroot()
        self.base_url = manifest.attrib['BaseUrl']
        if not self.base_url.startswith('https://cdn.unrealengine.com/'):
            raise ValueError('Unexpected Unreal dependency host')
        self.files = self.index(manifest.findall('Files/File'), 'Name')
        self.blobs = self.index(manifest.findall('Blobs/Blob'), 'Hash')
        self.packs = self.index(manifest.findall('Packs/Pack'), 'Hash')
        for digest in (*self.blobs, *self.packs):
            if re.fullmatch('[0-9a-f]{40}', digest) is None:
                raise ValueError(f'Invalid dependency hash: {digest}')

    @staticmethod
    def index(entries, key):
        result = {}
        for entry in entries:
            name = entry.attrib[key]
            if name in result:
                raise ValueError(f'Duplicate dependency entry: {name}')
            result[name] = entry.attrib
        return result

    @staticmethod
    def digest(path):
        with Path(path).open('rb') as stream:
            return hashlib.file_digest(stream, 'sha1').hexdigest()

    def restore(self, names, workers=4):
        names = sorted(set(names))
        groups = defaultdict(list)
        for name in names:
            entry = self.files[name]
            path = contained_path(self.engine, name)
            if path.exists():
                if not path.is_file() or self.digest(path) != entry['Hash']:
                    raise ValueError(f'Existing dependency differs from the Unreal manifest: {name}')
                continue
            blob = self.blobs[entry['Hash']]
            groups[blob['PackHash']].append((name, blob))
        print(f'Restoring {sum(len(items) for items in groups.values())} Unreal dependencies from {len(groups)} packs', flush=True)
        with ThreadPoolExecutor(max_workers=workers) as executor:
            list(executor.map(self.restore_pack, groups.items()))

    def restore_pack(self, group):
        digest, entries = group
        pack = self.packs[digest]
        compressed = self.cache / (digest + '.gz')
        pending = None
        unpacked = self.cache / (digest + '.' + uuid4().hex + '.unpacked')
        if not compressed.exists():
            print(f'Downloading Unreal pack {digest}: {pack["CompressedSize"]} bytes, {len(entries)} files', flush=True)
            pending = self.cache / (digest + '.' + uuid4().hex + '.download')
            url = '/'.join((self.base_url, pack['RemotePath'], digest))
            try:
                with urlopen(Request(url, headers={'User-Agent': 'UnrealProspero'}), timeout=60) as response, pending.open('xb') as stream:
                    while chunk := response.read1(1024 * 1024):
                        stream.write(chunk)
                if pending.stat().st_size != int(pack['CompressedSize']):
                    raise ValueError(f'Compressed dependency size mismatch: {digest}')
            except BaseException:
                pending.unlink(missing_ok=True)
                raise
        try:
            with gzip.open(pending if pending is not None else compressed, 'rb') as source, unpacked.open('xb') as output:
                while chunk := source.read(1024 * 1024):
                    output.write(chunk)
            if unpacked.stat().st_size != int(pack['Size']) or self.digest(unpacked) != digest:
                raise ValueError(f'Dependency pack hash mismatch: {digest}')
            if pending is not None:
                pending.replace(compressed)
                pending = None
            with unpacked.open('rb') as source:
                for name, blob in entries:
                    source.seek(int(blob['PackOffset']))
                    data = source.read(int(blob['Size']))
                    if len(data) != int(blob['Size']) or hashlib.sha1(data).hexdigest() != blob['Hash']:
                        raise ValueError(f'Dependency hash mismatch: {name}')
                    destination = contained_path(self.engine, name)
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    created = False
                    try:
                        with destination.open('xb') as output:
                            created = True
                            output.write(data)
                    except BaseException:
                        if created:
                            destination.unlink()
                        raise
                    if self.files[name].get('IsExecutable') == 'true' and os.name != 'nt':
                        destination.chmod(destination.stat().st_mode | 0o111)
        finally:
            unpacked.unlink(missing_ok=True)
            if pending is not None:
                pending.unlink(missing_ok=True)
