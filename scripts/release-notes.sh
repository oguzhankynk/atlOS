#!/bin/sh
# Print the release notes for a version: its CHANGELOG.md section plus how to
# boot the ISO. Used by .github/workflows/release.yml.
#
# Usage: scripts/release-notes.sh <version>      e.g. 0.2.0

set -e

VERSION=$1
ROOT=$(cd "$(dirname "$0")/.." && pwd)

if [ -z "$VERSION" ]; then
    echo "usage: $0 <version>" >&2
    exit 2
fi

# Lines after "## [VERSION]" up to the next "## [" heading or the link list
notes=$(awk -v heading="## [$VERSION]" '
    index($0, heading) == 1 { found = 1; next }
    found && (/^## \[/ || /^\[[^]]+\]: /) { exit }
    found { print }
' "$ROOT/CHANGELOG.md" | sed -e '/./,$!d')

if [ -z "$notes" ]; then
    echo "CHANGELOG.md has no section for $VERSION" >&2
    exit 1
fi

printf '%s\n\n---\n\n' "$notes"
printf 'Boot it with `qemu-system-x86_64 -cdrom atlOS-%s.iso -m 512M -serial stdio`.\n' "$VERSION"
printf 'Build from source: see [docs/building.md](https://github.com/oguzhankynk/atlOS/blob/v%s/docs/building.md).\n' "$VERSION"
