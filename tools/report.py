#!/usr/bin/env python3
"""Read benchmark run documents.

Every number a run produces lives in a JSON document written by the benchmark itself
(see docs/RESULTS.md). This script is the reader: it recomputes percentiles from the raw
histogram buckets rather than trusting the summary the run wrote, so the analysis here is
independent of the C++ that produced the file and can be changed without re-running
anything.

    tools/report.py summary  results/**/*.json
    tools/report.py compare  baseline.json candidate.json ...
    tools/report.py index    results/index.jsonl
    tools/report.py verify   results/**/*.json
    tools/report.py markdown receiver.json --sender sender.json
"""

from __future__ import annotations

import argparse
import glob
import json
import math
import sys
from pathlib import Path

PERCENTILES = (0.5, 0.9, 0.99, 0.999, 0.9999)
PERCENTILE_LABELS = ("p50", "p90", "p99", "p99.9", "p99.99")


# --- histograms ------------------------------------------------------------------


def bucket_to_cycles(bucket: int) -> int:
    """Mid-bucket cycle value. Mirrors HDRHistogram::BucketToCycles."""
    sub_bucket = bucket & 0x3F
    top_bucket = bucket >> 6
    if top_bucket == 0:
        return sub_bucket

    bit_position = top_bucket + 5
    significant_digit = 1 << bit_position
    precision_digits = sub_bucket << (bit_position - 6)
    mid_bucket = 0 if top_bucket <= 1 else 1 << (bit_position - 7)
    return significant_digit | precision_digits | mid_bucket


class Histogram:
    """One recorded distribution, reconstructed from the sparse buckets."""

    def __init__(self, name: str, raw: dict, ns_per_cycle: float):
        self.name = name
        self.ns_per_cycle = ns_per_cycle
        self.samples = raw["samples"]
        self.total_cycles = raw["total_cycles"]
        self.clipped = raw["clipped"]
        self.max_cycles = raw["max_cycles"]
        self.buckets = [(int(i), int(n)) for i, n in raw["buckets"]]
        self.stored_summary = raw.get("summary_ns", {})

    @property
    def mean_ns(self) -> float:
        if self.samples == 0:
            return math.nan
        return self.total_cycles / self.samples * self.ns_per_cycle

    @property
    def max_ns(self) -> float:
        return self.max_cycles * self.ns_per_cycle

    def percentile_ns(self, p: float) -> float:
        """The bucket the p-th sample falls in, as the run's own reader would find it."""
        if self.samples == 0:
            return math.nan
        target = p * self.samples
        count = 0
        for index, occurrences in self.buckets:
            count += occurrences
            if count >= target:
                return bucket_to_cycles(index) * self.ns_per_cycle
        # Only reachable when clipped samples, which live in no bucket, push the target past everything recorded in range.
        return bucket_to_cycles(self.buckets[-1][0]) * self.ns_per_cycle if self.buckets else math.nan

    def percentiles_ns(self) -> list[float]:
        return [self.percentile_ns(p) for p in PERCENTILES]


class Run:
    """One benchmark run document."""

    def __init__(self, path: Path, document: dict):
        self.path = path
        self.doc = document
        self.ns_per_cycle = document.get("timing", {}).get("ns_per_cycle", 0.0)
        self.histograms = {
            name: Histogram(name, raw, self.ns_per_cycle)
            for name, raw in document.get("histograms", {}).items()
        }
        self.by_type = {
            name: Histogram(name, raw, self.ns_per_cycle)
            for name, raw in document.get("message_type_histograms", {}).items()
        }

    @property
    def run_id(self) -> str:
        return self.doc.get("run_id", self.path.stem)

    @property
    def benchmark(self) -> str:
        return self.doc.get("benchmark", "?")

    @property
    def status(self) -> str:
        return self.doc.get("status", "?")

    def metric(self, name: str, default=None):
        return self.doc.get("metrics", {}).get(name, default)

    def label(self) -> str:
        build = self.doc.get("build", {})
        sha = build.get("git_sha", "?")
        if build.get("git_dirty"):
            sha += "+dirty"
        return f"{self.run_id}  [{sha}]"


def load(path: str | Path) -> Run:
    path = Path(path)
    with path.open(encoding="utf-8") as handle:
        return Run(path, json.load(handle))


def expand(patterns: list[str]) -> list[Path]:
    """Expands globs on platforms whose shell does not, and keeps the order stable."""
    paths: list[Path] = []
    for pattern in patterns:
        matches = sorted(glob.glob(pattern, recursive=True))
        paths.extend(Path(m) for m in (matches or [pattern]))
    return paths


# --- formatting ------------------------------------------------------------------


def fmt_ns(value: float) -> str:
    if value is None or (isinstance(value, float) and math.isnan(value)):
        return "-"
    if value < 1_000:
        return f"{value:.1f} ns"
    if value < 1_000_000:
        return f"{value / 1_000:.1f} us"
    return f"{value / 1_000_000:.2f} ms"


def fmt_count(value) -> str:
    return f"{value:,}" if isinstance(value, (int, float)) else str(value)


def table(rows: list[list[str]], headers: list[str], aligns: str = "") -> str:
    widths = [len(h) for h in headers]
    for row in rows:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(cell))
    aligns = (aligns + "l" * len(headers))[: len(headers)]

    def line(cells):
        out = []
        for cell, width, align in zip(cells, widths, aligns):
            out.append(cell.ljust(width) if align == "l" else cell.rjust(width))
        return "  ".join(out).rstrip()

    return "\n".join([line(headers), line(["-" * w for w in widths])] + [line(r) for r in rows])


# --- commands --------------------------------------------------------------------


def command_summary(args) -> int:
    rows = []
    for path in expand(args.paths):
        run = load(path)
        for name, histogram in run.histograms.items():
            rows.append(
                [
                    run.run_id,
                    run.benchmark if name == next(iter(run.histograms)) else "",
                    name,
                    fmt_count(histogram.samples),
                    fmt_ns(histogram.mean_ns),
                    *[fmt_ns(v) for v in histogram.percentiles_ns()],
                    fmt_ns(histogram.max_ns),
                    fmt_count(run.metric("throughput_msgs_s", "") and round(run.metric("throughput_msgs_s"))),
                ]
            )
        if run.status != "ok":
            print(f"{run.run_id}: {run.status}: {run.doc.get('error', '')}", file=sys.stderr)

    headers = ["run", "bench", "histogram", "samples", "mean", *PERCENTILE_LABELS, "max", "msgs/s"]
    print(table(rows, headers, aligns="lll" + "r" * (len(headers) - 3)))
    return 0


def command_compare(args) -> int:
    baseline = load(args.baseline)
    print(f"baseline: {baseline.label()}")
    print()

    for path in expand(args.candidates):
        candidate = load(path)
        print(candidate.label())
        rows = []
        for name, histogram in candidate.histograms.items():
            reference = baseline.histograms.get(name)
            if reference is None:
                continue
            for label, value, base in zip(
                (*PERCENTILE_LABELS, "max", "mean"),
                (*histogram.percentiles_ns(), histogram.max_ns, histogram.mean_ns),
                (*reference.percentiles_ns(), reference.max_ns, reference.mean_ns),
            ):
                delta = "-" if not base else f"{(value - base) / base * 100:+.1f}%"
                rows.append([name, label, fmt_ns(base), fmt_ns(value), delta])

        for metric in ("throughput_msgs_s", "ring_full_stalls", "max_ring_depth"):
            base = baseline.metric(metric)
            value = candidate.metric(metric)
            if base is None or value is None:
                continue
            delta = "-" if not base else f"{(value - base) / base * 100:+.1f}%"
            rows.append(["metrics", metric, f"{base:,.0f}", f"{value:,.0f}", delta])

        print(table(rows, ["histogram", "stat", "baseline", "candidate", "delta"], aligns="ll rrr"))
        print()
    return 0


def command_index(args) -> int:
    rows = []
    with open(args.path, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            entry = json.loads(line)
            rows.append(
                [
                    entry.get("started_at", ""),
                    entry.get("benchmark", ""),
                    entry.get("run_id", ""),
                    entry.get("status", ""),
                    f"{entry.get('m_throughput_msgs_s', 0):,.0f}",
                    fmt_ns(entry.get("h_overall_p50_ns", math.nan)),
                    fmt_ns(entry.get("h_overall_p99_ns", math.nan)),
                    fmt_ns(entry.get("h_overall_p99_9_ns", math.nan)),
                    fmt_ns(entry.get("h_overall_max_ns", math.nan)),
                ]
            )
    headers = ["started", "bench", "run", "status", "msgs/s", "p50", "p99", "p99.9", "max"]
    print(table(rows, headers, aligns="llll" + "r" * 5))
    return 0


def command_verify(args) -> int:
    """Recomputes every summary from the buckets and reports any that disagree.

    The summary block is a convenience the run wrote for human eyes; this is the check
    that it never quietly disagrees with the distribution underneath it.
    """
    failures = 0
    for path in expand(args.paths):
        run = load(path)
        for histogram in list(run.histograms.values()) + list(run.by_type.values()):
            stored = histogram.stored_summary
            if not stored:
                continue
            recomputed = dict(
                zip(("p50", "p90", "p99", "p99_9", "p99_99"), histogram.percentiles_ns())
            )
            recomputed["mean"] = histogram.mean_ns
            recomputed["max"] = histogram.max_ns
            for key, value in recomputed.items():
                written = stored.get(key)
                if written is None:
                    continue
                if math.isnan(value) and written == 0:
                    continue
                # %.12g in the document, so agreement to a part in 1e-9 is exact here.
                if written == 0 and value == 0:
                    continue
                if abs(written - value) > max(abs(value), abs(written)) * 1e-9:
                    print(
                        f"{run.run_id} {histogram.name}.{key}: document says {written}, "
                        f"buckets say {value}",
                        file=sys.stderr,
                    )
                    failures += 1
        print(f"{run.run_id}: {'ok' if failures == 0 else 'MISMATCH'} ({run.path})")
    return 1 if failures else 0


def markdown_histogram_table(histograms: list[Histogram]) -> list[str]:
    lines = [
        "| Histogram | samples | mean | p50 | p90 | p99 | p99.9 | max |",
        "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |",
    ]
    for histogram in histograms:
        p50, p90, p99, p99_9, _ = histogram.percentiles_ns()
        lines.append(
            f"| {histogram.name} | {histogram.samples:,} | {fmt_ns(histogram.mean_ns)} | "
            f"{fmt_ns(p50)} | {fmt_ns(p90)} | {fmt_ns(p99)} | {fmt_ns(p99_9)} | "
            f"{fmt_ns(histogram.max_ns)} |"
        )
    return lines


def command_markdown(args) -> int:
    """Renders the README block. The README stops being a place numbers are typed."""
    run = load(args.path)
    build = run.doc.get("build", {})
    host = run.doc.get("host", {})
    timing = run.doc.get("timing", {})

    sha = build.get("git_sha", "?") + ("+dirty" if build.get("git_dirty") else "")
    machine = host.get("machine_type") or host.get("cpu_model") or host.get("hostname", "?")

    lines = [
        f"_Run `{run.run_id}` &middot; commit `{sha}` &middot; {machine} &middot; "
        f"{run.doc.get('started_at', '')}_",
        "",
    ]

    if run.benchmark == "offline_lob":
        lines += [
            f"* Throughput (msgs/sec): {run.metric('throughput_msgs_s', 0):,.0f}",
            f"* Total Time (ns): {run.metric('elapsed_ns', 0):,}",
            f"* Total Messages (msgs): {run.metric('total_messages', 0):,}",
        ]
    else:
        sent = load(args.sender) if args.sender else None
        lines += [f"* Total Time (ns): {run.metric('elapsed_ns', 0):,}"]
        if sent is not None:
            packets_sent = sent.metric("packets_sent", 0)
            messages_sent = sent.metric("messages_sent", 0)
            packets_received = run.metric("moldudp64_packets", 0)
            messages_received = run.metric("total_messages", 0)
            lines += [
                f"* MOLDUDP64 Packets sent (packets): {packets_sent:,}",
                f"* MOLDUDP64 Packets received (packets): {packets_received:,}",
                f"* MOLDUDP64 Packet Drop Rate: "
                f"{(packets_sent - packets_received) / packets_sent:.3e}" if packets_sent else "",
                f"* Messages sent (msgs): {messages_sent:,}",
                f"* Messages received (msgs): {messages_received:,}",
                f"* Message Drop Rate: "
                f"{(messages_sent - messages_received) / messages_sent:.3e}" if messages_sent else "",
            ]
        else:
            lines += [f"* Messages received (msgs): {run.metric('total_messages', 0):,}"]
        lines += [
            f"* Ring full stalls (RX waiting on the parse thread): {run.metric('ring_full_stalls', 0):,}",
            f"* Max ring depth (slots): {run.metric('max_ring_depth', 0):,}",
        ]

    drift = timing.get("ns_per_cycle_drift", 0.0)
    lines += [
        f"* ns_per_cycle (ns/cycle): {run.ns_per_cycle:.6g} (drift over the run: {drift:+.2e})",
        "",
    ]

    lines += markdown_histogram_table(list(run.histograms.values()))
    lines += ["", "<details><summary>Per message type</summary>", ""]
    lines += markdown_histogram_table(
        sorted(run.by_type.values(), key=lambda h: -h.samples)
    )
    lines += ["", "</details>"]

    print("\n".join(line for line in lines if line is not None))
    return 0


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("summary", help="one row per histogram per run")
    p.add_argument("paths", nargs="+")
    p.set_defaults(func=command_summary)

    p = sub.add_parser("compare", help="percentile deltas against a baseline run")
    p.add_argument("baseline")
    p.add_argument("candidates", nargs="+")
    p.set_defaults(func=command_compare)

    p = sub.add_parser("index", help="tabulate an index.jsonl")
    p.add_argument("path")
    p.set_defaults(func=command_index)

    p = sub.add_parser("verify", help="recompute every summary from the raw buckets")
    p.add_argument("paths", nargs="+")
    p.set_defaults(func=command_verify)

    p = sub.add_parser("markdown", help="render the README metrics block")
    p.add_argument("path")
    p.add_argument("--sender", help="the sender run document, for drop rates")
    p.set_defaults(func=command_markdown)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
