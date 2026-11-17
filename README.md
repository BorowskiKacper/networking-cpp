# HFT Feed Handler and Limit Order Book (in progress)
## Most important offline Metrics:
* Throughput: Total Messages Processed / Total Time Taken
* Processsing Time per Message (The Offline Latency): Since I cannot measure network latency from my laptop, I'm measuring Internal processing latency.
* Maximum Latency (The Jitter Test): 99th and 99.9th percentile processing times.
* Cache Misses (The Diagnostic Metric): Mimimize cache misses as much as possible to maximize throughput.

<!-- OFFLINE METRICS_START -->
_Run `20261116T162221Z-fd861735-140da10c` &middot; commit `fd861735+dirty` &middot; Latitude 5320 &middot; 2026-11-16T16:22:21Z_

* Throughput (msgs/sec): 7,724,510
* Total Time (ns): 34,791,174,074
* Total Messages (msgs): 268,744,780
* ns_per_cycle (ns/cycle): 0.554078 (drift over the run: -4.91e-07)

| Histogram | samples | mean | p50 | p90 | p99 | p99.9 | max |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| overall | 268,734,779 | 107.5 ns | 60.9 ns | 205.0 ns | 740.2 ns | 873.2 ns | 234.3 us |

<details><summary>Per message type</summary>

| Histogram | samples | mean | p50 | p90 | p99 | p99.9 | max |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 117,145,568 | 110.7 ns | 65.9 ns | 213.9 ns | 740.2 ns | 882.1 ns | 234.3 us |
| D | 114,360,997 | 101.3 ns | 58.2 ns | 189.5 ns | 722.5 ns | 837.8 ns | 157.5 us |
| U | 21,639,067 | 159.4 ns | 129.1 ns | 260.4 ns | 802.3 ns | 926.4 ns | 145.0 us |
| E | 5,722,824 | 90.5 ns | 41.0 ns | 191.7 ns | 713.7 ns | 855.5 ns | 33.6 us |
| I | 4,024,315 | 9.4 ns | 8.9 ns | 12.2 ns | 13.9 ns | 15.0 ns | 7.7 us |
| X | 2,787,676 | 56.4 ns | 29.4 ns | 118.0 ns | 580.7 ns | 793.4 ns | 58.7 us |
| F | 1,485,888 | 106.7 ns | 63.7 ns | 202.8 ns | 731.4 ns | 891.0 ns | 16.6 us |
| P | 1,218,602 | 12.4 ns | 12.2 ns | 13.9 ns | 15.0 ns | 21.6 ns | 4.0 us |
| L | 215,161 | 8.9 ns | 8.9 ns | 9.4 ns | 10.0 ns | 11.1 ns | 3.5 us |
| C | 99,917 | 64.4 ns | 36.0 ns | 134.6 ns | 598.4 ns | 802.3 ns | 3.8 us |
| Q | 17,836 | 12.3 ns | 12.2 ns | 13.9 ns | 15.0 ns | 22.7 ns | 53.2 ns |
| Y | 8,466 | 8.9 ns | 8.9 ns | 9.4 ns | 10.5 ns | 11.6 ns | 12.2 ns |
| H | 8,419 | 8.9 ns | 8.9 ns | 9.4 ns | 10.5 ns | 13.9 ns | 17.2 ns |
| J | 34 | 12.4 ns | 12.2 ns | 13.9 ns | 13.9 ns | 13.9 ns | 13.9 ns |
| S | 5 | 15.7 us | 17.5 us | 21.1 us | 21.1 us | 21.1 us | 21.2 us |
| K | 3 | 12.6 ns | 12.7 ns | 12.7 ns | 12.7 ns | 12.7 ns | 12.7 ns |
| V | 1 | 12.2 ns | 12.2 ns | 12.2 ns | 12.2 ns | 12.2 ns | 12.2 ns |

</details>
<!-- OFFLINE METRICS_END -->

### Online Metrics:
<!-- ONLINE METRICS_START -->
* Total Time (ns): 1145804717678
* MOLDUDP64 Packets sent (packets): 5746639
* MOLDUDP64 Packets received (packets): 5746478
* MOLDUDP64 Packet Drop Rate: 0
* Messages sent (msgs): 268744780
* Messages received (msgs): 268737166
* Message Drop Rate: 0
* Ring full stalls (RX waiting on the parse thread): 0
* Max ring depth (slots): 63
#### Overall: 
* Processing Time per Message (ns/msg): 359.011
* total (msgs): 268734779
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 134.641
    * p90: 387.854
    * p99: 970.744
    * p99.9: 4273.05
    * max: 262483
    * clipped (msgs): 0

#### End to end (socket to parsed): 
* Processing Time per Message (ns/msg): 163902
* total (msgs): 268734779
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 61560.2
    * p90: 212198
    * p99: 442553
    * p99.9: 739857
    * max: 1.90556e+06
    * clipped (msgs): 0
ns_per_cycle: 0.554078
Again find ns_per_cycle: 0.554078

#### msg_type S: 
* Processing Time per Message (ns/msg): 28522.4
* total (msgs): 5
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 15106.4
    * p90: 17517.7
    * p99: 17517.7
    * p99.9: 17517.7
    * max: 17485
    * clipped (msgs): 0

#### msg_type R: 
* Processing Time per Message (ns/msg): -nan
* total (msgs): 0
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 0
    * p90: 0
    * p99: 0
    * p99.9: 0
    * max: 0
    * clipped (msgs): 0

#### msg_type A: 
* Processing Time per Message (ns/msg): 310.052
* total (msgs): 117145568
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 92.5309
    * p90: 316.932
    * p99: 899.822
    * p99.9: 3918.44
    * max: 262483
    * clipped (msgs): 0

#### msg_type F: 
* Processing Time per Message (ns/msg): 252.975
* total (msgs): 1485888
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 55.4078
    * p90: 285.904
    * p99: 864.361
    * p99.9: 4166.66
    * max: 54895.8
    * clipped (msgs): 0

#### msg_type E: 
* Processing Time per Message (ns/msg): 390.824
* total (msgs): 5722824
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 140.182
    * p90: 441.046
    * p99: 1023.94
    * p99.9: 4716.31
    * max: 65950.2
    * clipped (msgs): 0

#### msg_type C: 
* Processing Time per Message (ns/msg): 242.099
* total (msgs): 99917
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 46.5425
    * p90: 339.095
    * p99: 926.418
    * p99.9: 1710.99
    * max: 15233.3
    * clipped (msgs): 0

#### msg_type X: 
* Processing Time per Message (ns/msg): 237.957
* total (msgs): 2787676
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 46.5425
    * p90: 330.23
    * p99: 908.687
    * p99.9: 1710.99
    * max: 99978.3
    * clipped (msgs): 0

#### msg_type D: 
* Processing Time per Message (ns/msg): 401.95
* total (msgs): 114360997
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 160.682
    * p90: 410.017
    * p99: 997.34
    * p99.9: 4716.31
    * max: 154292
    * clipped (msgs): 0

#### msg_type U: 
* Processing Time per Message (ns/msg): 497.471
* total (msgs): 21639067
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 229.388
    * p90: 489.805
    * p99: 1050.53
    * p99.9: 6063.82
    * max: 181212
    * clipped (msgs): 0
<!-- ONLINE METRICS_END -->


## Running the benchmarks

Every run writes one self-contained JSON document -- build provenance, host description,
the full configuration, the metrics and the raw histogram buckets -- rather than printing
numbers to be parsed back. See [docs/RESULTS.md](docs/RESULTS.md) for the schema, the
`results/` layout, and `tools/report.py`.

```sh
make clean && make offline-release
./build/apps/offline --input=itchmessages/12302019.NASDAQ_ITCH50 --cpu=3
tools/report.py summary "results/**/*.json"
```

The metric blocks above are rendered from those documents by `./update_readme_offline.sh`
and `./update_readme_online.sh`, not typed by hand.

## Naming Convention used
* Files: snake_case (all lowercase with _ (underscores) as separators between words)
* Functions: PascalCase (Each word starts with an uppercase letter)
* Variables: snake_case
* Class: PascalCase

* c++ files: .cc
* header files: .h
* .inc?

* namespace: fh_lob

Order storage can be selected at startup in both `offline` and `receiver`:

```bash
./build/apps/offline --input itchmessages/your-feed --lob.order_storage=pool
./build/apps/offline --input itchmessages/your-feed --lob.order_storage=indexed
```

`pool` (the default) uses a 4-byte reference lookup entry plus reusable live-order
storage. `indexed` stores each order at its reference-number index, eliminating
that lookup dependency. It uses approximately 20 bytes per configured reference
(6.2 GB for 310 million entries), ignores `lob.order_pool_capacity`, and supports
at most `UINT32_MAX` reference slots to retain compact 32-bit queue links.
One LimitOrderBook class stores the CLI selection in a bool and uses runtime
if/else branches for order access, allocation, and deletion. The selected mode is included in the run configuration.

Empty price levels are retained by default. Both `offline` and `receiver` accept
`--lob.recycle_empty_levels=true` to erase empty levels from their price maps and
return their slots to the price-level pool:

```bash
./build/apps/offline --input itchmessages/your-feed --lob.recycle_empty_levels=true
./build/apps/offline --input itchmessages/your-feed --lob.recycle_empty_levels=false
```

With recycling enabled, `lob.price_level_pool_capacity` must cover peak concurrent
levels across all stocks. With recycling disabled, it must cover every distinct
stock-locate/price pair seen during the session. Both settings keep the pool's
preallocated memory reserved. The flag is independent of `lob.order_storage` and
is recorded in each run's configuration.

Run the focused storage correctness checks with `make order-storage-test`.
