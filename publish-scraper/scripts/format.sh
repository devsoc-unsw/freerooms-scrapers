#!/usr/bin/env bash

set -euo pipefail

root_dir="$(
    cd "$(dirname "${BASH_SOURCE[0]}")/.."
    pwd
)"

cd "$root_dir"

clang_format="${CLANG_FORMAT:-clang-format}"

if ! command -v "$clang_format" >/dev/null 2>&1; then
    echo "Error: $clang_format is not installed."
    echo "Install clang-format and try again."
    exit 1
fi

mapfile -d '' code_files < <(
    find src include tests \
        -type f \
        \( \
            -name '*.c' \
            -o -name '*.cc' \
            -o -name '*.cpp' \
            -o -name '*.cxx' \
            -o -name '*.h' \
            -o -name '*.hh' \
            -o -name '*.hpp' \
            -o -name '*.hxx' \
        \) \
        -print0 \
        | sort -z
)

if ((${#code_files[@]} == 0)); then
    echo "No C++ files found."
    exit 0
fi

echo "Formatting ${#code_files[@]} C++ files..."

"$clang_format" \
    -i \
    "${code_files[@]}"

echo "Formatting complete."