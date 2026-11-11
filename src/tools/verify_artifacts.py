# Python / verify_artifacts.py / Python 3.10+ / read-only artifact provenance check
"""Compare deployed files without loading binaries, rebuilding or synchronizing copies."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import sys


def inspect_file(path):
    path = Path(path)
    result = {'path': str(path)}
    try:
        before = path.stat()
        digest = hashlib.sha256()
        with path.open('rb') as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b''):
                digest.update(chunk)
        after = path.stat()
        if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
            result.update(status='changed_during_read')
        else:
            result.update(status='present', size=after.st_size, sha256=digest.hexdigest().upper())
    except FileNotFoundError:
        result['status'] = 'missing'
    except OSError as error:
        result.update(status='unreadable', error=str(error))
    return result


def compare_group(paths):
    files = [inspect_file(path) for path in paths]
    hashes = {item['sha256'] for item in files if item['status'] == 'present'}
    if any(item['status'] != 'present' for item in files):
        status = 'incomplete'
    elif len(hashes) != 1:
        status = 'different'
    else:
        status = 'identical'
    return dict(status=status, files=files)


def reject_nonfinite(value):
    raise ValueError('Non-finite JSON number: ' + value)


def inspect_json(path):
    result = inspect_file(path)
    if result['status'] == 'present':
        try:
            # Report validity only. Configuration values may be private.
            with Path(path).open('r', encoding='utf-8-sig') as stream:
                data = json.load(stream, parse_constant=reject_nonfinite)
            if not isinstance(data, dict):
                raise ValueError('Expected a JSON object.')
            result['json_valid'] = True
        except (OSError, UnicodeError, ValueError) as error:
            result.update(json_valid=False, error=str(error))
    return result


def collect(root, deploy):
    root, deploy = Path(root).resolve(), Path(deploy).resolve()
    groups = {
        'internal_dll': [root / 'HazzureInternal/build/Hazzure.Internal.dll',
                         root / 'release/modules/Hazzure.Internal.dll',
                         root / 'Hazzure/modules/Hazzure.Internal.dll',
                         deploy / 'Hazzure.Internal.dll'],
        'panel': [root / 'HazzurePanel/Hazzure.panel.exe', root / 'release/Hazzure.panel.exe'],
        'runtime': [root / 'Hazzure/Hazzure.exe', root / 'release/Hazzure.exe'],
        'external': [root / 'HazzureExternal/build/HazzureExternal.exe',
                     root / 'release/HazzureExternal.exe'],
        'client_settings': [root / 'Hazzure/settings.json', root / 'release/settings.json'],
    }
    compared = {name: compare_group(paths) for name, paths in groups.items()}
    configs = [inspect_json(path) for path in [root / 'Hazzure/settings.json',
               root / 'release/settings.json', deploy / 'hazzure_config.json']]
    issues = [name + ': ' + group['status'] for name, group in compared.items()
              if group['status'] != 'identical']
    issues += ['configuration: ' + item['path'] for item in configs
               if item['status'] != 'present' or not item.get('json_valid')]
    return {
        'schema_version': 1,
        'captured_at_utc': datetime.now(timezone.utc).isoformat(),
        'root': str(root), 'deploy': str(deploy),
        'verification': 'file bytes and JSON syntax only; no runtime or source equivalence claim',
        'groups': compared, 'configurations': configs, 'issues': issues,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument('--deploy', type=Path, default=Path('C:/HazzureKernel'))
    parser.add_argument('--output', type=Path)
    args = parser.parse_args(argv)
    result = collect(args.root, args.deploy)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
        print('Manifest: ' + str(args.output.resolve()))
        for issue in result['issues']:
            print('[ISSUE] ' + issue)
    else:
        print(json.dumps(result, indent=2, ensure_ascii=False))
    return 2 if result['issues'] else 0


if __name__ == '__main__':
    sys.exit(main())
