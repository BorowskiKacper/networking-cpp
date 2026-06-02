# HFT Feed Handler and Limit Order Book (in progress)
## Most important offline Metrics:
* Throughput: Total Messages Processed / Total Time Taken
* Processsing Time per Message (The Offline Latency): Since I cannot measure network latency from my laptop, I'm measuring Internal processing latency.
* Maximum Latency (The Jitter Test): 99th and 99.9th percentile processing times.
* Cache Misses (The Diagnostic Metric): Mimimize cache misses as much as possible to maximize throughput.

## Current Scores:
* Throughput (msgs/sec): 1.28038e+07
    * Total Time (ns): 20989387292
    * Total Messages (msgs): 268744780
* Processing Time per Message (ns/msg): 68.6877
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 18.2846
        * p90: 55.4078
        * p99: 414.45
        * p99.9: 1214.54
        * max: 1.48401e+06
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
