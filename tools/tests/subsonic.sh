#!/usr/bin/env bash
set -euo pipefail
repo="$(cd "$(dirname "$0")/../.." && pwd)"
out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT
cxx="${CXX:-c++}"
# A macOS SDK supplies libxml2 and curl; Linux uses their pkg-config packages.
if [[ "$(uname)" == Darwin ]]; then
    flags=(-I"$(xcrun --show-sdk-path)/usr/include/libxml2" -lxml2 -lcurl)
else
    read -r -a flags <<< "$(pkg-config --cflags --libs libxml-2.0 libcurl)"
fi
read -r -a avflags <<< "$(pkg-config --cflags --libs libavutil)"
"$cxx" -std=c++20 -Wall -Wextra -pthread -DGF_SUBSONIC "$repo/tools/tests/subsonic.cpp" \
    "$repo/source/subsonic.cpp" "${flags[@]}" "${avflags[@]}" -o "$out/subsonic"
cd "$out"
python3 "$repo/tools/tests/subsonic_server.py" "$out/subsonic"
