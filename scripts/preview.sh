#!/bin/bash
# Draw weather/Display.h on the Mac and write a PNG (no M5Paper needed).
#   scripts/preview.sh [nodata] [out.png]
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
B="$(mktemp -d)"
cp "$HERE"/../weather/{Display,Font,Icons}.h "$B"/
cp "$HERE"/preview/Data.h "$HERE"/preview/preview.cpp "$B"/
(cd "$B" && clang++ -std=c++17 -O1 -w -o preview preview.cpp && ./preview "${1:-ok}" out.pgm)
python3 -c "
from PIL import Image
Image.open('$B/out.pgm').save('${2:-$HERE/../preview.png}')"
echo "wrote ${2:-$HERE/../preview.png}"
