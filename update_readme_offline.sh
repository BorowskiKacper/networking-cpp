#!/usr/bin/env bash
# usage: ./update_readme_offline.sh results/bench=offline_lob/date=2026-09-06/<run_id>.json
#
# The offline replay writes its numbers to a run document (docs/RESULTS.md); this only renders one into the README block. Nothing here parses program output.
set -euo pipefail

README="README.md"
PYTHON="${PYTHON:-python3}"

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <offline run document>.json" >&2
    exit 2
fi

result="$1"
[[ -f "$result" ]] || { echo "$0: no such run document: $result" >&2; exit 2; }

metrics="$("$PYTHON" tools/report.py markdown "$result")"

# swap the new metrics in between the two markers, leaving the rest of the README unchanged
awk -v m="$metrics" '
    /<!-- OFFLINE METRICS_START -->/ { print; printf "%s\n", m; skip=1; next }
    /<!-- OFFLINE METRICS_END -->/   { skip=0 }
    !skip
' "$README" > README.tmp && mv README.tmp "$README"

echo "README.md updated from $result"
