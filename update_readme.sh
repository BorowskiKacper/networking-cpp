#!/usr/bin/env bash
# usage: ./update_readme.sh < output.txt
#    or: ./build/apps/offline 5 2>&1 | ./update_readme.sh
set -euo pipefail

README="README.md"

input="$(cat)"

# extract from first "* Throughput" line through the last "* ns_per_cycle (ns/cycle):" line
metrics="$(printf '%s\n' "$input" | awk '
    /^\* Throughput/                     { capture=1 }
    capture                              { buf = buf $0 "\n" }
    /^    \* ns_per_cycle \(ns\/cycle\):/ { end = buf }
    END { printf "%s", end }
')"

# swap the new metrics in between the two markers, leaving the rest of the README unchanged
awk -v m="$metrics" '
    /<!-- METRICS_START -->/ { print; printf "%s\n", m; skip=1; next }
    /<!-- METRICS_END -->/   { skip=0 }
    !skip
' "$README" > README.tmp && mv README.tmp "$README"

echo "README.md updated."