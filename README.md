# HFT Feed Handler and Limit Order Book (in progress)
## Most important offline Metrics:
* Throughput: Total Messages Processed / Total Time Taken
* Processsing Time per Message (The Offline Latency): Since I cannot measure network latency from my laptop, I'm measuring Internal processing latency.
* Maximum Latency (The Jitter Test): 99th and 99.9th percentile processing times.
* Cache Misses (The Diagnostic Metric): Mimimize cache misses as much as possible to maximize throughput.

<!-- OFFLINE METRICS_START -->
* Throughput (msgs/sec): 6.89575e+06
    * Total Time (ns): 38972544173
    * Total Messages (msgs): 268744780
* Processing Time per Message (ns/msg): 223.041
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 74.8005
        * p90: 240.47
        * p99: 784.574
        * p99.9: 953.014
        * max: 232487
    * total (msgs): 268734779
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078
ns_per_cycle: 0.554078
Again find ns_per_cycle: 0.554078

msg_type: S
* Processing Time per Message (ns/msg): 11074.6
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 8900.71
        * p90: 9432.62
        * p99: 9432.62
        * p99.9: 9432.62
        * max: 9381.09
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
* Processing Time per Message (ns/msg): 198.02
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 64.8271
        * p90: 202.793
        * p99: 740.248
        * p99.9: 926.418
        * max: 232487
    * total (msgs): 117145568
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: F
* Processing Time per Message (ns/msg): 165.538
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 43.7722
        * p90: 182.846
        * p99: 713.652
        * p99.9: 1515.96
        * max: 73963.9
    * total (msgs): 1485888
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: E
* Processing Time per Message (ns/msg): 238.618
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 89.2066
        * p90: 262.633
        * p99: 811.17
        * p99.9: 961.879
        * max: 31864.5
    * total (msgs): 5722824
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: C
* Processing Time per Message (ns/msg): 146.857
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 33.7988
        * p90: 193.927
        * p99: 713.652
        * p99.9: 882.092
        * max: 15158.5
    * total (msgs): 99917
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: X
* Processing Time per Message (ns/msg): 145.303
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 33.7988
        * p90: 187.278
        * p99: 713.652
        * p99.9: 899.823
        * max: 25257.1
    * total (msgs): 2787676
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: D
* Processing Time per Message (ns/msg): 242.656
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 99.18
        * p90: 251.551
        * p99: 802.305
        * p99.9: 935.284
        * max: 162409
    * total (msgs): 114360997
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078

msg_type: U
* Processing Time per Message (ns/msg): 317.42
* Maximum Latency: 
    * Overall Histogram (ns/msg): 
        * p50: 140.182
        * p90: 290.337
        * p99: 837.766
        * p99.9: 1285.46
        * max: 148679
    * total (msgs): 21639067
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554078
<!-- OFFLINE METRICS_END -->

<!-- ONLINE METRICS_START -->
<!-- ONLINE METRICS_END -->


## Naming Convention used
* Files: snake_case (all lowercase with _ (underscores) as separators between words)
* Functions: PascalCase (Each word starts with an uppercase letter)
* Variables: snake_case
* Class: PascalCase

* c++ files: .cc
* header files: .h
* .inc?

* namespace: fh_lob
