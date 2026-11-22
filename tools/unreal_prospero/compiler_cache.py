import json
import os
from pathlib import Path

from .process import run


def run_ubt(arguments, engine, work, log, environment):
    launcher = os.environ.get('UNREALPROSPERO_CCACHE')
    if launcher is None:
        return run(arguments, engine, work, log, environment)
    launcher = Path(launcher).resolve(strict=True)
    if not launcher.is_file():
        raise ValueError('The compiler cache launcher must be a file')
    actions = Path(work) / 'editor-actions.json'
    actions.unlink(missing_ok=True)
    run([*arguments, '-WriteOutdatedActions=' + str(actions)], engine, work, Path(log).with_name('editor-plan.log'), environment)
    document = json.loads(actions.read_text(encoding='utf-8-sig'))
    by_id = {action['Id']: action for action in document['Actions']}
    if len(by_id) != len(document['Actions']):
        raise ValueError('Duplicate editor action identifiers')
    count = 0
    for action in document['Actions']:
        inputs = set(action.get('PrerequisiteItems', []))
        for prerequisite in action['PrerequisiteActions']:
            products = by_id[prerequisite]['ProducedItems']
            if not products:
                raise ValueError('An editor prerequisite action has no ordering outputs')
            inputs.update(products)
        action['PrerequisiteItems'] = sorted(inputs)
        compiler = action['CommandPath']
        if action['Type'] != 'Compile' or Path(compiler).name.lower() not in ('cl.exe', 'clang', 'clang.exe', 'clang++', 'clang++.exe', 'clang-cl', 'clang-cl.exe', 'gcc', 'g++'):
            continue
        if '"' in compiler:
            raise ValueError('Invalid compiler path')
        action['CommandArguments'] = '"' + compiler + '" ' + action['CommandArguments']
        action['CommandPath'] = str(launcher)
        action['PrerequisiteItems'].append(str(launcher))
        action['bCanExecuteInUBA'] = False
        action['bCanExecuteRemotely'] = False
        count += 1
    if not count:
        raise ValueError('The editor action graph contains no cacheable compiler commands')
    actions.write_text(json.dumps(document, indent=2) + '\n', encoding='utf-8')
    print(f'Using ccache for {count} editor compilation actions', flush=True)
    return run([*arguments, '-Mode=Execute', '-Actions=' + str(actions)], engine, work, log, environment)
