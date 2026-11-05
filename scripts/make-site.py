#!/usr/bin/env python3
"""Build the documentation site (MkDocs) into build/site/html.

The pages stay where they are in the repository (the subsystem READMEs next
to their code). This script copies every Markdown file into build/site/src
under the same path, so links between pages keep working, and points the
links to anything else (source files, directories) at GitHub, at the given
git ref. A README's page also gets a link to its directory's source. Then MkDocs builds docs/mkdocs.yml with --strict: a broken link
fails the build.

Usage: scripts/make-site.py [--ref REF]
  REF  the tag or commit the source links point at (default: HEAD's commit)
Needs MkDocs: pip install -r docs/requirements.txt
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "build", "site")
SRC = os.path.join(OUT, "src")
REPO_URL = "https://github.com/oguzhankynk/atlOS"

LINK = re.compile(r"(!?\[[^\]]*\]\()([^)\s]+)(\))")
FENCE = re.compile(r"^\s*(```|~~~)")


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def pages():
    files = git("ls-files", "--cached", "--others", "--exclude-standard", "*.md").split("\n")
    return [f for f in files if not f.startswith("tools/disk/files/")]


def images():
    return git("ls-files", "docs/images").split("\n")


def rewrite_link(target, page, copied, ref):
    """Return the link for the site: unchanged if it reaches a copied file,
    a GitHub URL if it reaches anything else in the repository."""
    if re.match(r"^[a-z]+:", target) or target.startswith("#"):
        return target
    path, _, anchor = target.partition("#")
    repo_path = os.path.normpath(os.path.join(os.path.dirname(page), path))
    if repo_path in copied:
        return target
    if repo_path.startswith("..") or not os.path.exists(os.path.join(ROOT, repo_path)):
        sys.exit(f"{page}: link to a missing file: {target}")
    kind = "tree" if os.path.isdir(os.path.join(ROOT, repo_path)) else "blob"
    url = f"{REPO_URL}/{kind}/{ref}/{repo_path}"
    return url + ("#" + anchor if anchor else "")


def rewrite_page(text, page, copied, ref):
    lines = []
    in_code = False
    for line in text.split("\n"):
        if FENCE.match(line):
            in_code = not in_code
        elif not in_code:
            line = LINK.sub(lambda m: m.group(1) + rewrite_link(m.group(2), page, copied, ref) + m.group(3), line)
        lines.append(line)
    return "\n".join(lines)


def add_source_link(text, page, ref):
    """Put a link to the README's directory under its title."""
    directory = os.path.dirname(page)
    if os.path.basename(page) != "README.md" or not directory:
        return text
    title, _, rest = text.partition("\n")
    return f"{title}\n\nSource: [`{directory}/`]({REPO_URL}/tree/{ref}/{directory})\n{rest}"


def main():
    parser = argparse.ArgumentParser(description="Build the documentation site.")
    parser.add_argument("--ref", default=None, help="tag or commit for the source links")
    args = parser.parse_args()
    ref = args.ref or git("rev-parse", "HEAD")

    mkdocs = shutil.which("mkdocs")
    if not mkdocs:
        sys.exit("mkdocs not found: pip install -r docs/requirements.txt")

    shutil.rmtree(OUT, ignore_errors=True)
    copied = set(pages()) | set(images())
    for page in pages():
        with open(os.path.join(ROOT, page), encoding="utf-8") as f:
            text = add_source_link(rewrite_page(f.read(), page, copied, ref), page, ref)
        os.makedirs(os.path.dirname(os.path.join(SRC, page)), exist_ok=True)
        with open(os.path.join(SRC, page), "w", encoding="utf-8") as f:
            f.write(text)
    for image in images():
        os.makedirs(os.path.dirname(os.path.join(SRC, image)), exist_ok=True)
        shutil.copy(os.path.join(ROOT, image), os.path.join(SRC, image))

    built = subprocess.run([mkdocs, "build", "--strict", "-f", os.path.join(ROOT, "docs", "mkdocs.yml")])
    if built.returncode != 0:
        sys.exit(built.returncode)
    print(f"site in {os.path.join(OUT, 'html')}")


if __name__ == "__main__":
    main()
