#!/usr/bin/env bash
# usage: ./build/apps/receiver 239.0.0.1 12345 3 2>&1 | ./update_readme_online.sh
#    or: ./update_readme_online.sh < receiver_output.txt
#
# Start the sender in another terminal: ./build/apps/sender 239.0.0.1 12345
#
# The sent counts are taken from the values already in README.md's ONLINE block,
# since the sender always replays the same ITCH file. After changing that file,
# seed the new totals once:
#   PACKETS_SENT=5746639 MESSAGES_SENT=268744780 ./update_readme_online.sh < ...
set -euo pipefail

README="README.md"

input="$(cat)"

die() {
    echo "update_readme_online: $*" >&2
    exit 1
}

# the receiver prints a banner line, then the value on the line after it
read_after_banner() {
    printf '%s\n' "$input" | awk -v banner="$1" '
        found                  { print; exit }
        index($0, banner) == 1 { found = 1 }
    '
}

require_count() {
    [[ "$1" =~ ^[0-9]+$ ]] || die "$2"
}

time_ns="$(read_after_banner '=====TIME TAKEN=====')"
messages_received="$(read_after_banner '=====TOTAL MESSAGE COUNT=====')"
packets_received="$(read_after_banner '=====TOTAL MOLDUDP64 MESSAGES')"

require_count "$time_ns" "no '=====TIME TAKEN=====' value in the receiver output (did it exit normally?)"
require_count "$messages_received" "no '=====TOTAL MESSAGE COUNT=====' value in the receiver output"
require_count "$packets_received" "no '=====TOTAL MOLDUDP64 MESSAGES' value in the receiver output"

# read the sent counts back out of the ONLINE block, before we overwrite it
readme_field() {
    awk -v pat="$1" '
        /<!-- ONLINE METRICS_START -->/ { inblock = 1; next }
        /<!-- ONLINE METRICS_END -->/   { inblock = 0 }
        inblock && $0 ~ pat {
            n = $0; sub(/^[^:]*:[[:space:]]*/, "", n); gsub(/[^0-9]/, "", n); print n
        }
    ' "$README" | tail -n 1
}

packets_sent="${PACKETS_SENT:-$(readme_field '^\* MOLDUDP64 Packets sent \(packets\):')}"
messages_sent="${MESSAGES_SENT:-$(readme_field '^\* Messages sent \(msgs\):')}"

[[ "$packets_sent" =~ ^[1-9][0-9]*$ ]] ||
    die "no '* MOLDUDP64 Packets sent (packets): <N>' in the $README ONLINE block; set PACKETS_SENT= to seed it"
[[ "$messages_sent" =~ ^[1-9][0-9]*$ ]] ||
    die "no '* Messages sent (msgs): <N>' in the $README ONLINE block; set MESSAGES_SENT= to seed it"

read -r packet_drop_rate message_drop_rate <<<"$(awk \
    -v ps="$packets_sent" -v pr="$packets_received" \
    -v ms="$messages_sent" -v mr="$messages_received" \
    'BEGIN { printf "%.6e %.6e\n", (ps - pr) / ps, (ms - mr) / ms }')"

# extract from the "#### Overall:" header through the last "* ns_per_cycle (ns/cycle):" line
histograms="$(printf '%s\n' "$input" | awk '
    /^#### Overall:/                { capture=1 }
    capture                              { buf = buf $0 "\n" }
    /^    \* clipped \(msgs\):/ { end = buf }
    END { printf "%s", end }
')"

[[ -n "$histograms" ]] || die "no histogram output found in the receiver output"

metrics="$(
    cat <<EOF
* Total Time (ns): $time_ns
* MOLDUDP64 Packets sent (packets): $packets_sent
* MOLDUDP64 Packets received (packets): $packets_received
* MOLDUDP64 Packet Drop Rate: $packet_drop_rate
* Messages sent (msgs): $messages_sent
* Messages received (msgs): $messages_received
* Message Drop Rate: $message_drop_rate
$histograms
EOF
)"

# swap the new metrics in between the two markers, leaving the rest of the README unchanged
awk -v m="$metrics" '
    /<!-- ONLINE METRICS_START -->/ { print; printf "%s\n", m; skip=1; next }
    /<!-- ONLINE METRICS_END -->/   { skip=0 }
    !skip
' "$README" > README.tmp && mv README.tmp "$README"

echo "README.md updated."
