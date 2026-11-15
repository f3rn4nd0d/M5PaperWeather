#!/bin/bash
# Draw one page of the M5Paper on the Mac and write a PNG (no device needed).
#   scripts/preview.sh <page: 0 weather+header | 1 weather clean | 2 Home Assistant> [out.png] [nodata|noha]
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
B="$(mktemp -d)"
cp "$HERE"/../weather/{Display,DisplayA,DisplayB,DisplayHome,Canvas,Font,Icons,Logo,LogoRetro}.h "$B"/
cp "$HERE"/preview/{Data.h,M5EPD.h,preview.cpp} "$B"/
OUT="${2:-$HERE/../preview.png}"
(cd "$B" && clang++ -std=c++17 -O1 -w -I. -o preview preview.cpp && ./preview "${1:-0}" out.pgm "$3" 2>/dev/null)
python3 -c "
from PIL import Image
Image.open('$B/out.pgm').save('$OUT')"
echo "wrote $OUT"
