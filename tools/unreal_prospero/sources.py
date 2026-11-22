from pathlib import Path
import subprocess

from .process import run


def checkout(specification, destination, sparse, work):
    destination = Path(destination).resolve()
    revision = specification['revision']
    if destination.exists():
        actual = subprocess.check_output(['git', '-C', str(destination), 'rev-parse', 'HEAD'], text=True).strip()
        if actual != revision:
            raise ValueError(f'Unexpected source revision in {destination}: {actual}')
        subprocess.run(['git', '-C', str(destination), 'diff', '--quiet', '--ignore-space-at-eol', 'HEAD'], check=True)
        untracked = subprocess.check_output(['git', '-C', str(destination), 'ls-files', '--others', '--exclude-standard'], text=True)
        if untracked.strip():
            raise ValueError(f'Unexpected untracked source files in {destination}: {untracked}')
        return destination
    destination.parent.mkdir(parents=True, exist_ok=True)
    logs = Path(work) / 'logs'
    run(['git', 'clone', '--filter=blob:none', '--depth=1', '--no-checkout', specification['repository'], destination], destination.parent, work, logs / (destination.name + '-clone.log'))
    run(['git', '-C', destination, 'config', 'core.longpaths', 'true'], destination.parent, work, logs / (destination.name + '-git-config.log'))
    if sparse:
        run(['git', '-C', destination, 'sparse-checkout', 'set', '--cone', *sparse], destination.parent, work, logs / (destination.name + '-sparse.log'))
    run(['git', '-C', destination, 'fetch', '--depth=1', 'origin', revision], destination.parent, work, logs / (destination.name + '-fetch.log'))
    run(['git', '-C', destination, 'checkout', '--detach', revision], destination.parent, work, logs / (destination.name + '-checkout.log'))
    actual = subprocess.check_output(['git', '-C', str(destination), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != revision:
        raise ValueError(f'Source checkout did not select {revision}')
    return destination
