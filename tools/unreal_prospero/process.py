import os
from pathlib import Path
import shutil
import subprocess


def require_tool(name):
    path = shutil.which(name)
    if path is None:
        raise FileNotFoundError(f'Required tool is not on PATH: {name}')
    return str(Path(path).resolve(strict=True))


def environment(work):
    work = Path(work).resolve()
    directories = {name: work / name for name in ('temp', 'dotnet', 'nuget', 'logs', 'ddc', 'zen')}
    for directory in directories.values():
        directory.mkdir(parents=True, exist_ok=True)
    result = dict(os.environ)
    result.update(TEMP=str(directories['temp']), TMP=str(directories['temp']), TMPDIR=str(directories['temp']), DOTNET_CLI_HOME=str(directories['dotnet']), NUGET_PACKAGES=str(directories['nuget']), UE_LogFolder=str(directories['logs']))
    result['UE-LocalDataCachePath'] = str(directories['ddc'])
    result['UE-ZenDataPath'] = str(directories['zen'])
    return result


def run(arguments, directory, work, log):
    arguments = [str(value) for value in arguments]
    print(subprocess.list2cmdline(arguments), flush=True)
    log = Path(log)
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open('wb') as output:
        result = subprocess.run(arguments, cwd=directory, env=environment(work), stdout=output, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        print(log.read_text(encoding='utf-8', errors='replace')[-16000:], flush=True)
        raise subprocess.CalledProcessError(result.returncode, arguments)
    return log
