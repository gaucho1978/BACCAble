"""Exercise release packaging and tag protection without contacting GitHub."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPTS = Path(__file__).resolve().parent

class ReleaseTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.env = dict(os.environ, GITHUB_REPOSITORY='test/repo', GITHUB_STEP_SUMMARY='')

    def command(self, *args, ok=True):
        result = subprocess.run(args, cwd=self.root, env=self.env, capture_output=True, text=True)
        self.assertEqual(result.returncode == 0, ok, result.stdout + result.stderr)
        return result.stdout.strip()

    def artifacts(self, flags=''):
        source = self.root / 'release'
        source.mkdir()
        for board in ('C1', 'C2', 'BH', 'CAN'):
            if board == 'CAN' and flags:
                continue
            for ext in ('bin', 'hex', 'elf'):
                (source / f'baccable-{board}.{ext}').write_bytes(b'firmware fixture')
            (source / f'build-info-{board}.json').write_text(json.dumps(dict(
                flavor=board, commit='abc', version='beta-abc', extra_cppflags=flags)))

    def package(self, flags='', ok=True):
        self.command('python3', str(SCRIPTS / 'package-release.py'), 'beta', 'v5-beta-test',
                     'beta-abc', 'abc', flags, ok=ok)

    def test_checksums_and_identity(self):
        self.artifacts()
        self.package()
        files = (self.root / 'dist/SHA256SUMS').read_text().splitlines()
        self.assertEqual(len(files), 13)
        for line in files:
            digest, name = line.split('  ')
            self.assertEqual(hashlib.sha256((self.root / 'dist' / name).read_bytes()).hexdigest(), digest)
        self.assertTrue((self.root / 'dist/baccableC1_beta.elf').is_file())

    def test_mismatched_source_rejected(self):
        self.artifacts()
        path = self.root / 'release/build-info-BH.json'
        path.write_text(path.read_text().replace('abc', 'wrong'))
        self.package(ok=False)

    def test_missing_firmware_rejected(self):
        self.artifacts()
        (self.root / 'release/baccable-C2.bin').unlink()
        self.package(ok=False)

    def test_debug_without_can(self):
        self.artifacts('-DDEBUG_MODE')
        self.package('-DDEBUG_MODE')
        self.assertEqual(len(list((self.root / 'dist').iterdir())), 11)

    def test_immutable_and_rolling_tags(self):
        self.command('git', 'init', '-q')
        self.command('git', 'config', 'user.name', 'CI test')
        self.command('git', 'config', 'user.email', 'ci@example.invalid')
        self.command('git', 'commit', '-qm', 'first', '--allow-empty')
        first = self.command('git', 'rev-parse', 'HEAD')
        self.command('git', 'init', '--bare', '-q', 'remote.git')
        self.command('git', 'remote', 'add', 'origin', str(self.root / 'remote.git'))
        fake = self.root / 'bin'
        fake.mkdir()
        gh = fake / 'gh'
        gh.write_text('#!/bin/sh\nprintf "%s\\n" "$PUBLISHED"\nexit "${GH_EXIT:-0}"\n')
        gh.chmod(0o755)
        self.env['PATH'] = str(fake) + os.pathsep + self.env['PATH']
        script = str(SCRIPTS / 'release-tag.sh')
        self.command('bash', script, 'publish', 'beta', 'v5-beta-test')
        self.command('git', 'commit', '-qm', 'second', '--allow-empty')
        self.command('bash', script, 'publish', 'beta', 'v5-beta-test', ok=False)
        self.assertIn(first, self.command('git', 'ls-remote', 'origin', 'refs/tags/v5-beta-test'))
        self.command('bash', script, 'publish', 'beta', 'continuous')
        self.command('git', 'commit', '-qm', 'third', '--allow-empty')
        self.command('bash', script, 'publish', 'beta', 'continuous')
        self.assertIn(self.command('git', 'rev-parse', 'HEAD'), self.command('git', 'ls-remote', 'origin', 'refs/tags/continuous'))
        self.env['PUBLISHED'] = 'v5-beta-new'
        self.command('bash', script, 'validate', 'beta', 'v5-beta-new', ok=False)
        self.env['GH_EXIT'] = '1'
        self.command('bash', script, 'validate', 'stable', 'v5.0.0', ok=False)
        self.command('bash', script, 'validate', 'beta', 'v5.0.0', ok=False)

if __name__ == '__main__':
    unittest.main()
