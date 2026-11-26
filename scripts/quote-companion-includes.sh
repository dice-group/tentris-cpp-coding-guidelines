#!/usr/bin/env bash
# Write the companion header include of a source file in the "" form.
#
# The companion header of `dir/Foo.cpp` is `dir/Foo.hpp` (or `.h`, `.hh`, `.hxx`).
# The script changes `#include <some/path/Foo.hpp>` to `#include "Foo.hpp"` only if:
#   - the header file is in the same directory as the source file,
#   - the header file name has the same stem as the source file (case-sensitive),
#   - the path of the header file ends with `some/path/Foo.hpp`.
# Other includes stay as they are. Run clang-format after this script to sort the includes.
#
# Usage:
#   quote-companion-includes.sh [--check] PATH...
# A PATH is a source file or a directory. The script searches a directory recursively,
# also untracked and build directories. To change only tracked files, use:
#   git ls-files -z '*.cpp' | xargs -0 quote-companion-includes.sh
# With --check, the script writes no file. It prints the changes and exits with 1 if there are changes.
# Exit status 2 means an error. The script runs with bash 3.2 (macOS) and later.

set -euo pipefail

usage() {
    sed -n '2,17s/^# \{0,1\}//p' "$0"
    exit 2
}

fail() {
    printf 'error: %s\n' "$1" >&2
    exit 2
}

check=0
if [ "${1:-}" = "--check" ]; then
    check=1
    shift
fi
[ "$#" -gt 0 ] || usage

angle_include='^([[:space:]]*#[[:space:]]*include[[:space:]]*)<([^>]+)>(.*)$'

# Print the source files in the arguments, one for each line.
source_files() {
    local path
    for path in "$@"; do
        if [ -d "$path" ]; then
            find "$path" -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \) | LC_ALL=C sort
        else
            case "$path" in
                *.c | *.cc | *.cpp | *.cxx) printf '%s\n' "$path" ;;
            esac
        fi
    done
}

# Change the companion include of one source file. Return 0 if the file has a companion include in the <> form.
process() {
    local source="$1"
    local dir base stem real_dir number_and_line number line prefix include_path rest name new_line tmp
    [ -f "$source" ] && [ -r "$source" ] || fail "cannot read $source"
    dir=$(dirname "$source")
    base=$(basename "$source")
    stem=${base%%.*}
    real_dir=$(cd "$dir" && pwd -P) || fail "cannot open the directory $dir"

    while IFS= read -r number_and_line; do
        number=${number_and_line%%:*}
        line=${number_and_line#*:}
        [[ $line =~ $angle_include ]] || continue
        prefix=${BASH_REMATCH[1]}
        include_path=${BASH_REMATCH[2]}
        rest=${BASH_REMATCH[3]}
        name=${include_path##*/}
        case "$name" in
            *.h | *.hh | *.hpp | *.hxx) ;;
            *) continue ;;
        esac
        [ "${name%.*}" = "$stem" ] || continue
        [ -f "$dir/$name" ] || continue
        [[ "$real_dir/$name" == */"$include_path" ]] || continue

        new_line="${prefix}\"${name}\"${rest}"
        printf '%s:%s: %s  ->  %s\n' "$source" "$number" "$line" "$new_line"
        if [ "$check" -eq 0 ]; then
            # `set -e` does not apply in a function that runs as an `if` condition, so check each step.
            tmp=$(mktemp) || fail "cannot make a temporary file"
            if ! NUMBER="$number" NEW_LINE="$new_line" \
                awk 'NR == ENVIRON["NUMBER"] + 0 { print ENVIRON["NEW_LINE"]; next } { print }' "$source" > "$tmp" \
                || ! cat "$tmp" > "$source"; then
                rm -f "$tmp"
                fail "cannot write $source"
            fi
            rm -f "$tmp"
        fi
        return 0
    done < <(grep -n -E '^[[:space:]]*#[[:space:]]*include[[:space:]]*<' "$source" || true)
    return 1
}

changed=0
while IFS= read -r source; do
    if process "$source"; then
        changed=$((changed + 1))
    fi
done < <(source_files "$@")

if [ "$check" -eq 1 ] && [ "$changed" -gt 0 ]; then
    exit 1
fi
exit 0
