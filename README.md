# HFT Feed Handler and Limit Order Book (in progress)
## Most important Metrics:
* Throughput: Total Messages Processed / Total Time Taken
* Processsing Time per Message (The Offline Latency): Since I cannot measure network latency from my laptop, I'm measuring Internal processing latency.
* Maximum Latency (The Jitter Test): 99th and 99.9th percentile processing times.
* Cache Misses (The Diagnostic Metric): Mimimize cache misses as much as possible to maximize throughput.

## My Scores:
* Throughput: 0.0017462198864493 msgs/ns
    * time taken to send all messages: 141885060938 ns
    * total message count sent: 268744780
    * total message count received: 247762515
    * total MoldUDP64 packets received: 5306532
* Processing Time per Message: N/A
    * Overall Histogram: 
        * Samples: 182456541
        * Mean: 758
        * p50: 18446744073709551615 = -1
        * p90: 18446744073709551615
        * p99: 18446744073709551615
        * p99.9: 18446744073709551615
        * max: 20796992
        * clipped: 0
    * ns_per_cycle: 0
* Maximum Latency: N/A
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
