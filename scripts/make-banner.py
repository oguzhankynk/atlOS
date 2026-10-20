#!/usr/bin/env python3
"""Turn docs/images/logo.jpg into the boot banner of kernel/core/banner.cpp:
half block characters (two pixels per character cell, about square on the
VGA screen's 9x16 cells), the left part in one color, the rest in another.

Usage: scripts/make-banner.py [width] > lines.txt   (prints the C++ lines)
"""

import sys

from PIL import Image

WIDTH = int(sys.argv[1]) if len(sys.argv) > 1 else 72
BOX = (215, 270, 1545, 610)  # the drawn part of the image
THRESHOLD = 70  # alpha above this is "on"

alpha = Image.open("docs/images/logo.jpg").convert("RGBA").split()[3].crop(BOX)
height = round(alpha.height / alpha.width * WIDTH * 9 / 8)
height += height % 2
small = alpha.resize((WIDTH, height), Image.BOX)

rows = []
for y in range(0, height, 2):
    cells = []
    for x in range(WIDTH):
        top = small.getpixel((x, y)) > THRESHOLD
        bottom = small.getpixel((x, y + 1)) > THRESHOLD
        cells.append("█" if top and bottom else "▀" if top else "▄" if bottom else " ")
    rows.append(cells)

# The picture and the name are split at the first empty column after a quarter of the width
split = next(x for x in range(WIDTH // 4, WIDTH) if all(row[x] == " " for row in rows))
for cells in rows:
    left, right = "".join(cells[:split]), "".join(cells[split:]).rstrip()
    print(f'    PICTURE "{left}" NAME "{right}" RESET,')
