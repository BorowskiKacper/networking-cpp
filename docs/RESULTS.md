# Benchmark results

Every run of `offline`, `receiver` or `sender` writes one self-contained JSON document.
That document is the interface: it carries what was built, where it ran, what it was
configured with, what it measured, and the raw histogram buckets behind every number.
Program output on stderr is a courtesy for watching a run go by, and nothing downstream
parses it.

## Running

Settings are `--key=value` options; `--help` lists every key with its type and default.

```sh
make clean && make offline-release
./build/apps/offline --input=itchmessages/12302019.NASDAQ_ITCH50 --cpu=3

# online: sender in one terminal, receiver in another
./build/apps/receiver --multicast_ip=239.0.0.1 --port=12345 --rx_cpu=2 --parse_cpu=3
./build/apps/sender   --multicast_ip=239.0.0.1 --port=12345 --input=itchmessages/12302019.NASDAQ_ITCH50
```

Output plumbing is deliberately kept out of the configuration, so two runs that differ
only in where they wrote are still the same configuration:

| option | default | meaning |
| --- | --- | --- |
| `--out_dir` | `results` | root of the run-document tree; empty writes no file |
| `--out` | | exact path, overriding the `out_dir` layout |
| `--index` | `results/index.jsonl` | one-line-per-run summary to append to; empty disables |
| `--stdout_json` | off | also write the document to stdout on one line |
| `--quiet` | off | suppress the human-readable summary on stderr |

## Layout

```
results/
  bench=offline_lob/date=2026-09-06/20260906T215337Z-31134e3a-5c62c694.json
  bench=receiver/date=2026-09-06/...
  index.jsonl
```

The run id is `<UTC timestamp>-<commit>-<configuration hash>`: sortable, and two runs of
the same configuration are recognisable side by side in a listing. The hash covers every
configuration key and value, so changing `--recv_batch` changes the id while changing
`--out` does not.

The directory layout is hive-style on purpose. It is the same locally and in a bucket, so
publishing is a recursive copy with nothing rewritten on the way, and BigQuery can read
`bench` and `date` as partition columns from an external table over the prefix:

```sh
gsutil -m rsync -r results gs://<bucket>/runs
```

Documents are written to `<path>.partial` and renamed into place, so a sync or a reader
never picks up a half-written object. The id is unique down to the second, so two runs of
one configuration started inside the same second get a `-2`, `-3` suffix on the filename
rather than one silently overwriting the other; an explicit `--out` is always taken
literally.

## The document

```jsonc
{
  "schema_version": 1,
  "run_id": "20260906T215337Z-31134e3a-5c62c694",
  "benchmark": "offline_lob",
  "status": "ok",                  // "error" plus an "error" field if the run failed
  "started_at": "2026-09-06T21:53:37Z",
  "started_at_unix_ns": 1788731617000000000,
  "duration_s": 41.2,

  "build":  { "git_sha", "git_dirty", "config", "cxxflags", "compiler", "assertions" },
  "host":   { "hostname", "machine_type", "cpu_model", "logical_cpus", "kernel_release",
              "kernel_version", "thp_enabled", "thp_defrag", "smt",
              "governor": { "cpu3": "performance" } },
  "input":  { "path", "bytes", "modified_at", "fingerprint" },
  "config": { /* every --key=value, verbatim, nested one level on the dot */ },
  "timing": { "ns_per_cycle", "ns_per_cycle_end", "ns_per_cycle_drift" },
  "metrics":{ "throughput_msgs_s", "elapsed_ns", "total_messages", ... },

  "histograms": {
    "overall": {
      "samples": 100940, "total_cycles": 62734550, "clipped": 0, "max_cycles": 939000,
      "top_buckets": 32, "sub_buckets": 64,
      "summary_ns": { "mean", "p50", "p90", "p99", "p99_9", "p99_99", "max" },
      "buckets": [[164, 5000], [165, 5000], ...]
    }
  },
  "message_type_histograms": { "A": { ... }, "D": { ... } }
}
```

A few things are load-bearing:

- **`buckets` is the real result.** Sparse `[index, count]` pairs, in *cycles*. About 2000
  buckets are reserved and a few dozen are ever occupied, so the whole distribution costs
  about as much as the summary above it — and every percentile stays recomputable later,
  at a resolution no printed number preserves. Add `p99.99`, change the interpolation, or
  re-bucket a year-old run without re-running anything.
- **`summary_ns` is derived** from those buckets and `timing.ns_per_cycle`, and is present
  so a document reads at a glance. It is never the only copy of a number.
  `tools/report.py verify` recomputes it and reports any disagreement.
- **`config` is what the run read its settings out of**, serialized verbatim. The
  benchmark does not hold the configuration anywhere else, so the recorded configuration
  cannot drift from the one that ran. Keys marked fixed at build time (a template
  parameter such as `ring.capacity`) are recorded the same way but rejected on the command
  line.
- **`timing.ns_per_cycle_drift`** compares the calibrations taken before and after the
  run. Far from zero means the clock moved under the measurement and every nanosecond
  figure in the document is suspect.
- **`input.fingerprint`** hashes the length plus the first and last 64 KiB. Enough to
  notice a different input file, without a second pass over gigabytes that would also warm
  the page cache the benchmark is measuring against.
- **A failed run still writes a document**, with `status: "error"` and a message. A crash
  is a record, not a gap in the series.

## `index.jsonl`

One flat line per run appended by every run: identity, build, host, all metrics (`m_`),
all configuration (`cfg_`), and each top-level histogram's summary (`h_<name>_p99_ns` and
friends). Names are flat and underscore-separated so the file loads with no ETL:

```sh
bq load --source_format=NEWLINE_DELIMITED_JSON --autodetect runs.index results/index.jsonl
```

```python
import pandas as pd
runs = pd.read_json("results/index.jsonl", lines=True)
runs.groupby("cfg_recv_batch")["h_end_to_end_p99_9_ns"].agg(["mean", "std", "count"])
```

This is the A/B ledger: it answers "did that experiment beat the baseline across five
runs" without opening a single full document.

## Reading results

```sh
tools/report.py summary  "results/**/*.json"      # one row per histogram per run
tools/report.py compare  baseline.json cand.json  # percentile deltas
tools/report.py index    results/index.jsonl      # tabulate the ledger
tools/report.py verify   "results/**/*.json"      # recompute summaries from the buckets
tools/report.py markdown receiver.json --sender sender.json
```

`report.py` recomputes percentiles from the raw buckets rather than reading
`summary_ns`, so the analysis is independent of the C++ that produced the file.

The README metric blocks are rendered, not typed:

```sh
./update_readme_offline.sh results/bench=offline_lob/date=.../<run_id>.json
./update_readme_online.sh  <receiver run>.json <sender run>.json
```

Drop rates come from the sender's own document, so the sent totals are no longer carried
forward by hand.

## Alongside a run

`perf stat` output belongs next to the document it explains, under the same run id:

```sh
perf stat -j -e cycles,instructions,cache-misses,dTLB-load-misses,LLC-load-misses \
    -o results/bench=offline_lob/date=$(date -u +%F)/<run_id>.perf.json \
    ./build/apps/offline --input=... --cpu=3
```

On a Batch job, set `BENCH_MACHINE_TYPE` so the host block records the instance shape —
it is the one thing about a GCE host that is not discoverable from the filesystem.
