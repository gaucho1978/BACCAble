"""Record firmware identity and enforce configurable Flash/RAM budgets."""
import json
import os
from pathlib import Path
import subprocess

flavor = os.environ['FLAVOR']
base = Path('firmware/baccable/build') / flavor
elf = base / f'baccable-{flavor}.elf'
size = subprocess.check_output([os.getenv('SIZE_TOOL', 'arm-none-eabi-size'), str(elf)], text=True)
text, data, bss = map(int, size.splitlines()[1].split()[:3])
flash, ram = text + data, data + bss
flash_limit = int(os.getenv('FLASH_LIMIT') or (98304 if flavor == 'C1' else 65536))
ram_limit = int(os.getenv('RAM_LIMIT') or 16384)
info = dict(flavor=flavor, commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
            version=os.environ['VERSION'], extra_cppflags=os.getenv('EXTRA_CPPFLAGS', ''),
            compiler=subprocess.check_output([os.getenv('CC_TOOL', 'arm-none-eabi-gcc'), '--version'], text=True).splitlines()[0],
            flash_bytes=flash, ram_bytes=ram, flash_limit=flash_limit, ram_limit=ram_limit)
(base / f'build-info-{flavor}.json').write_text(json.dumps(info, indent=2) + '\n')
summary = f'### {flavor}\n\n| Resource | Bytes | Budget |\n|---|---:|---:|\n| Flash | {flash} | {flash_limit} |\n| Static RAM | {ram} | {ram_limit} |\n\nSource: `{info["commit"]}`; version: `{info["version"]}`.\n'
print(summary)
if os.getenv('GITHUB_STEP_SUMMARY'):
    with open(os.environ['GITHUB_STEP_SUMMARY'], 'a') as out:
        out.write(summary)
if flash > flash_limit or ram > ram_limit:
    raise SystemExit('Firmware exceeds configured size budget')
