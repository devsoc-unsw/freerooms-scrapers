#!/usr/bin/env bash

set -euo pipefail

root_dir="$(
    cd "$(dirname "${BASH_SOURCE[0]}")/.."
    pwd
)"

cd "$root_dir"

clang_format="${CLANG_FORMAT:-clang-format}"
clang_tidy="${CLANG_TIDY:-clang-tidy}"
build_dir="${BUILD_DIR:-build}"

for command in cmake "$clang_format" "$clang_tidy"; do
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "Error: $command is not installed."
        exit 1
    fi
done

if [[ -n "${LINT_JOBS:-}" ]]; then
    jobs="$LINT_JOBS"
elif command -v nproc >/dev/null 2>&1; then
    jobs="$(nproc)"
else
    jobs="4"
fi

echo "Configuring CMake..."

cmake \
    -S . \
    -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release

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

mapfile -d '' translation_units < <(
    find src tests \
        -type f \
        \( \
            -name '*.c' \
            -o -name '*.cc' \
            -o -name '*.cpp' \
            -o -name '*.cxx' \
        \) \
        -print0 \
        | sort -z
)

echo
echo "Checking formatting for ${#code_files[@]} files..."

"$clang_format" \
    --dry-run \
    --Werror \
    "${code_files[@]}"

echo "Formatting passed."

echo
echo "Running clang-tidy on ${#translation_units[@]} translation units using $jobs jobs..."

tmp_dir="$(mktemp -d)"

cleanup() {
    rm -rf "$tmp_dir"
}

trap cleanup EXIT

lint_file() {
    local file="$1"
    local safe_name
    local log_file

    safe_name="$(
        printf '%s' "$file" \
            | tr '/ ' '__'
    )"

    log_file="$tmp_dir/$safe_name.log"

    if ! "$clang_tidy" \
        --quiet \
        -p "$build_dir" \
        "$file" \
        >"$log_file" 2>&1
    then
        echo
        echo "Lint failed: $file"
        cat "$log_file"
        return 1
    fi
}

export -f lint_file
export clang_tidy
export build_dir
export tmp_dir

if ! printf '%s\0' "${translation_units[@]}" \
    | xargs \
        -0 \
        -n 1 \
        -P "$jobs" \
        bash -c 'lint_file "$1"' _
then
    echo
    echo "Linting failed."
    exit 1
fi

echo "clang-tidy passed."
echo
echo "Linting passed."