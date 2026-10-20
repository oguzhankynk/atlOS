# Versioning and releases

## Versions

atlOS uses [Semantic Versioning](https://semver.org/). While the major version
is 0 the project is experimental: a new feature bumps the minor version
(0.1.0 → 0.2.0), a fix the patch version (0.2.0 → 0.2.1). 1.0.0 is kept for a
milestone such as running a shell on its own.

The version is written in one place, the root `CMakeLists.txt`:

```cmake
project(atlOS VERSION 0.1.0 LANGUAGES C CXX ASM)
```

From it the build generates `core/version.hpp` (in the build tree, see
`cmake/generate-version.cmake`):

| Macro | Example |
|---|---|
| `ATLOS_VERSION_MAJOR` / `_MINOR` / `_PATCH` | `0` / `1` / `0` |
| `ATLOS_VERSION_STRING` | `"0.1.0"` |
| `ATLOS_GIT_DESCRIBE` | `"v0.1.0"`, `"v0.1.0-3-g266d2d3"`, `"v0.1.0-3-g266d2d3-dirty"` |

The banner prints both, e.g. `atlOS 0.1.0 (v0.1.0-3-g266d2d3)`: three commits
after the v0.1.0 tag, at commit 266d2d3; `-dirty` means uncommitted changes.
The header is rewritten only when its content changes, so the kernel is not
rebuilt for nothing, and there is no build time in the image (same source,
same binary). Doxygen gets the version through `ATLOS_VERSION`.

## Branches

- `development`: day to day work, the default branch
- `master`: released versions only; every commit on it is a release merge
  and carries a `vX.Y.Z` tag

## Making a release

1. On `development`, set the version in `CMakeLists.txt` and move the
   `[Unreleased]` entries of `CHANGELOG.md` under the new version with today's
   date (and update the links at the bottom).
2. Check: `scripts/build.sh clean all selftest crashtest shelltest format-check tidy`
   (CI runs the same on every push).
3. Commit: `git commit -am "Release vX.Y.Z"`.
4. Merge, tag and push:

   ```sh
   git checkout master
   git merge --no-ff development -m "Release vX.Y.Z"
   git tag -a vX.Y.Z -m "atlOS X.Y.Z"
   git push origin master development vX.Y.Z
   git checkout development
   ```

Pushing the tag starts the release workflow (`.github/workflows/release.yml`).
It checks that the tag matches `project(atlOS VERSION ...)`, builds and runs
the self tests from the tag (so the banner shows `vX.Y.Z`), and publishes the
GitHub release with the `CHANGELOG.md` section as notes
(`scripts/release-notes.sh`) and the ISO attached.

## Continuous integration

`.github/workflows/ci.yml` runs on every push to `development` and `master`
and on pull requests: build, self tests in QEMU (with the test disk, checked
afterwards by `fsck.fat`), the crash test, the shell test over the serial
port, `format-check` and `tidy` (any clang-tidy finding fails it). The ISO
and the test logs are kept as artifacts for 14 days. Both workflows run on `ubuntu-22.04`, the same tool
versions as the development machine, and share `.github/actions/setup`, which
builds the cross toolchain with `scripts/build-toolchain.sh` once and then
restores it from the cache (the key is the script's hash).
