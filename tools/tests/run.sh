#!/usr/bin/env bash
set -euo pipefail
repo="$(cd "$(dirname "$0")/../.." && pwd)"
out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT
cxx="${CXX:-c++}"
"$cxx" -std=c++20 -Wall -Wextra -pthread "$repo/tools/tests/storage.cpp" -o "$out/storage"
"$cxx" -std=c++20 -Wall -Wextra "$repo/tools/tests/tags.cpp" "$repo/source/library.cpp" -o "$out/tags"
"$cxx" -std=c++20 -Wall -Wextra -I "$repo/sysmodule/lib/helix-aac" "$repo/tools/tests/mp4.cpp" -o "$out/mp4"
cd "$out"
./storage
./tags
if command -v ffmpeg >/dev/null; then
    ffmpeg -v error -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=1' -ac 2 -c:a aac valid.m4a
    ./mp4 valid.m4a
else
    ./mp4
fi
echo 'Host regression checks passed'
