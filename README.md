# HFT Feed Handler and Limit Order Book (in progress)
## Most important offline Metrics:
* Throughput: Total Messages Processed / Total Time Taken
* Processsing Time per Message (The Offline Latency): Since I cannot measure network latency from my laptop, I'm measuring Internal processing latency.
* Maximum Latency (The Jitter Test): 99th and 99.9th percentile processing times.
* Cache Misses (The Diagnostic Metric): Mimimize cache misses as much as possible to maximize throughput.

## Current Scores:
* Throughput (msgs/sec): 1.9849e+06
    * Total Time (ns): 135394595936
    * Total Messages (msgs): 268744780
* Processing Time per Message (ns/msg): 824.374
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 321.365
        * p90: 793.44
        * p99: 1356.38
        * p99.9: 9148.94
        * max: 6.2868e+07
    * total (msgs): 268734779
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078
* Cache Misses: N/A

## Naming Convention used
* Files: snake_case (all lowercase with _ (underscores) as separators between words)
* Functions: PascalCase (Each word starts with an uppercase letter)
* Variables: snake_case
* Class: PascalCase

* c++ files: .cc
* header files: .h
* .inc?

* namespace: fh_lob
