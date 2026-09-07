#!/usr/bin/env bash
# usage: ./update_readme_online.sh <receiver run document>.json <sender run document>.json
#
# Both sides of the online benchmark write run documents (docs/RESULTS.md), so the drop rates come from the two counts that were actually measured -- the sent totals are no longer carried forward by hand from the previous README block.
set -euo pipefail

README="README.md"
PYTHON="${PYTHON:-python3}"

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <receiver run document>.json <sender run document>.json" >&2
    exit 2
fi

receiver="$1"
sender="$2"
for f in "$receiver" "$sender"; do
    [[ -f "$f" ]] || { echo "$0: no such run document: $f" >&2; exit 2; }
done

metrics="$("$PYTHON" tools/report.py markdown "$receiver" --sender "$sender")"

# swap the new metrics in between the two markers, leaving the rest of the README unchanged
awk -v m="$metrics" '
    /<!-- ONLINE METRICS_START -->/ { print; printf "%s\n", m; skip=1; next }
    /<!-- ONLINE METRICS_END -->/   { skip=0 }
    !skip
' "$README" > README.tmp && mv README.tmp "$README"

echo "README.md updated from $receiver + $sender"
