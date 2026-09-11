"""Keep legacy release filenames and attach source identity and file checksums."""
import hashlib
import json
import os
from pathlib import Path
import sys

kind, tag, version, commit, flags = sys.argv[1:]
assert kind in ('stable', 'beta')
source, target = Path('release'), Path('dist')
target.mkdir()  # Refuse to mix files with a previous packaging attempt.
boards = ['C1', 'C2', 'BH']
if '-DDEBUG_MODE' not in flags or kind == 'stable':
    boards.append('CAN')
records = []
for board in boards:
    info = json.loads((source / f'build-info-{board}.json').read_text())
    assert (info['commit'], info['version'], info['extra_cppflags']) == (commit, version, flags), 'Artifact identity mismatch'
    assert info['flavor'] == board
    records.append(info)
    for ext in ('bin', 'hex', 'elf'):
        name = f'baccable-{board}.{ext}'
        data = (source / name).read_bytes()
        assert data, f'Empty artifact: {name}'
        if ext == 'elf':
            name = f'baccable{board if board != "CAN" else "ActAsCanable"}_{kind}.elf'
        (target / name).write_bytes(data)
(target / 'BUILD_INFO.json').write_text(json.dumps(dict(tag=tag, commit=commit, version=version,
    workflow_run=f'{os.getenv("GITHUB_SERVER_URL", "https://github.com")}/{os.getenv("GITHUB_REPOSITORY", "")}/actions/runs/{os.getenv("GITHUB_RUN_ID", "")}',
    firmware=records), indent=2) + '\n')
checksums = ''.join(f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n' for path in sorted(target.iterdir()))
(target / 'SHA256SUMS').write_text(checksums)
if os.getenv('GITHUB_STEP_SUMMARY'):
    with open(os.environ['GITHUB_STEP_SUMMARY'], 'a') as out:
        out.write('### Release artifacts\n\n' + ''.join(f'- `{p.name}` ({p.stat().st_size} bytes)\n' for p in sorted(target.iterdir())))
