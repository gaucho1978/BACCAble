# CI and releases

`ci.yml` remains reusable by stable and beta publication. Host sanitizer tests
cover both display widths and existing transport/menu regressions. Their log is
an artifact and PASS lines appear in the job summary. Release script tests use
synthetic files and a temporary Git remote; they never publish to GitHub.

- Ordinary branch/PR CI cancels superseded runs. Release runs have a separate
  concurrency group and are not cancelled by ordinary CI.
- Markdown-only PRs skip lint and firmware builds, but still run host tests.
  Pushes and releases always build: a documentation push must not cancel a code
  run and then skip its validation. Unavailable comparison bases fail open to
  full testing. Require **CI result** in branch protection to cover lint as well
  as every firmware variant, including the documented skip case.
- Ubuntu is fixed to 24.04, cppcheck to `2.13.0-2ubuntu3` (one installation and
  four flavor checks), and ARM GCC to `15.3.Rel1`. Actions use commit SHAs with
  version comments. Update pins deliberately and rerun all checks. Ubuntu images
  and transitive packages still receive updates; this is not a hermetic build.
- Only publication jobs have `contents: write`. Test/build checkouts do not
  persist Git credentials. No PR job can publish or modify tags.

## Size budgets and artifact identity

Each build records compiler identity, source commit, version, extra preprocessor
flags and Flash/static RAM usage in `build-info-FLAVOR.json`. Flash is text +
data; static RAM is data + bss, excluding runtime stack/heap. Defaults match the
linker regions: C1 96 KiB Flash, other roles 64 KiB; RAM 16 KiB.

Repository variables `FLASH_LIMIT_C1`, `RAM_LIMIT_C1` (and C2/BH/CAN equivalents)
can set stricter byte budgets. Exceeding a budget fails CI. This is an absolute
budget, not a historical comparison; runtime memory headroom needs separate
assessment.

Releases keep the legacy ELF names plus BIN/HEX and add `BUILD_INFO.json` and
`SHA256SUMS`. Packaging verifies that every board has the requested commit,
version and flags. To verify downloaded assets in one directory:

```sh
sha256sum -c SHA256SUMS
# macOS:
shasum -a 256 -c SHA256SUMS
```

## Tag rules

Stable tags are `vX.Y.Z`. Beta names such as `v5-beta-3` are fixed versions.
Existing fixed tags must point at the requested source; already published fixed
releases are rejected instead of replacing their files. Choose a new version.
A failed run that created only a tag can be retried from the same commit.

Only `continuous` moves, using a Git force-with-lease check. All releases run
full reusable CI, including lint, before touching tags. The existing debug
configuration may omit the standalone CAN build; obsolete firmware assets are
removed from `continuous` when a role is omitted. Publication is not atomic:
a failed upload can leave a tag or partially populated release; inspect it before
recovery. Do not automatically delete or move a numbered version to recover.

## Deliberately deferred

Artifact attestations are useful for this public repository, but are not enabled
in this patch. They need `id-token: write` and `attestations: write` in the trusted
release path and validation on GitHub. Checksums detect corruption; they are not
signed provenance. The pinned ARM installer still downloads its versioned vendor
archive; archive-digest locking and reproducible runner images are separate work.

No binary cache, historical size comparison, or firmware behavior-diff framework
is introduced. Existing host regression tests are retained. Add small captured
traffic fixtures there when a real behavioral regression is found.

Local validation: `python3 .github/scripts/test-ci.py`,
`bash -n .github/scripts/release-tag.sh`, `actionlint`, and
`make -C tests -j2 test`.
