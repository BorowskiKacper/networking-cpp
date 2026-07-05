# HFT Feed Handler and Limit Order Book (in progress)
## Most important offline Metrics:
* Throughput: Total Messages Processed / Total Time Taken
* Processsing Time per Message (The Offline Latency): Since I cannot measure network latency from my laptop, I'm measuring Internal processing latency.
* Maximum Latency (The Jitter Test): 99th and 99.9th percentile processing times.
* Cache Misses (The Diagnostic Metric): Mimimize cache misses as much as possible to maximize throughput.

<!-- METRICS_START -->
* Throughput (msgs/sec): 4.44082e+06
    * Total Time (ns): 60516922091
    * Total Messages (msgs): 268744780
* Processing Time per Message (ns/msg): 336.246
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 127.992
        * p90: 347.961
        * p99: 890.957
        * p99.9: 2677.3
        * max: 551613
    * total (msgs): 268734779
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078
ns_per_cycle: 0.554078
Again find ns_per_cycle: 0.554078

msg_type: S
* Processing Time per Message (ns/msg): 48986.8
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 33900.7
        * p90: 43971.6
        * p99: 43971.6
        * p99.9: 43971.6
        * max: 43716.2
    * total (msgs): 5
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: R
* Processing Time per Message (ns/msg): -nan
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 0
        * p90: 0
        * p99: 0
        * p99.9: 0
        * max: 0
    * total (msgs): 0
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: A
* Processing Time per Message (ns/msg): 305.481
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 114.694
        * p90: 299.202
        * p99: 837.766
        * p99.9: 2535.46
        * max: 322958
    * total (msgs): 117145568
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: F
* Processing Time per Message (ns/msg): 256.427
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 74.8005
        * p90: 269.282
        * p99: 811.17
        * p99.9: 3173.76
        * max: 72965.9
    * total (msgs): 1485888
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: E
* Processing Time per Message (ns/msg): 351.824
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 130.208
        * p90: 401.152
        * p99: 935.283
        * p99.9: 2783.69
        * max: 544425
    * total (msgs): 5722824
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: C
* Processing Time per Message (ns/msg): 228.733
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 61.5026
        * p90: 290.337
        * p99: 828.9
        * p99.9: 1068.26
        * max: 20507.5
    * total (msgs): 99917
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: X
* Processing Time per Message (ns/msg): 224.437
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 61.5026
        * p90: 280.363
        * p99: 828.9
        * p99.9: 1214.54
        * max: 27985.9
    * total (msgs): 2787676
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: D
* Processing Time per Message (ns/msg): 360.133
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 145.168
        * p90: 374.557
        * p99: 917.553
        * p99.9: 2783.69
        * max: 551613
    * total (msgs): 114360997
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: U
* Processing Time per Message (ns/msg): 470.286
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 216.09
        * p90: 441.046
        * p99: 961.879
        * p99.9: 3882.98
        * max: 182492
    * total (msgs): 21639067
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078
<!-- METRICS_END -->

## Naming Convention used
* Files: snake_case (all lowercase with _ (underscores) as separators between words)
* Functions: PascalCase (Each word starts with an uppercase letter)
* Variables: snake_case
* Class: PascalCase

* c++ files: .cc
* header files: .h
* .inc?

* namespace: fh_lob
