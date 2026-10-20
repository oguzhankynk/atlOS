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
2. Check: `scripts/build.sh clean all selftest format-check tidy`.
3. Commit: `git commit -am "Release vX.Y.Z"`.
4. Merge and tag:

   ```sh
   git checkout master
   git merge --no-ff development -m "Release vX.Y.Z"
   git tag -a vX.Y.Z -m "atlOS X.Y.Z"
   git push origin master development vX.Y.Z
   git checkout development
   ```

5. Build the ISO from the tag (the banner then shows `vX.Y.Z`) and publish
   the release with the changelog section as notes:

   ```sh
   scripts/build.sh clean all
   gh release create vX.Y.Z build/default/atlOS.iso#atlOS-X.Y.Z.iso \
       --title "atlOS X.Y.Z" --notes-file <changelog section>
   ```
