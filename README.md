# HFT Feed Handler and Limit Order Book (in progress)
## Most important offline Metrics:
* Throughput: Total Messages Processed / Total Time Taken
* Processsing Time per Message (The Offline Latency): Since I cannot measure network latency from my laptop, I'm measuring Internal processing latency.
* Maximum Latency (The Jitter Test): 99th and 99.9th percentile processing times.
* Cache Misses (The Diagnostic Metric): Mimimize cache misses as much as possible to maximize throughput.

## Current Scores:
* Throughput (msgs/sec): 4.19677e+06
    * Total Time (ns): 64036029731
    * Total Messages (msgs): 268744780
* Processing Time per Message (ns/msg): 387.937
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 133.533
        * p90: 365.691
        * p99: 917.552
        * p99.9: 4379.43
        * max: 3.28273e+07
    * total (msgs): 268734779
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077
ns_per_cycle: 0.554077
Again find ns_per_cycle: 0.554076

msg_type: S
* Processing Time per Message (ns/msg): 60634
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 41134.7
        * p90: 47943.2
        * p99: 47943.2
        * p99.9: 47943.2
        * max: 48025.2
    * total (msgs): 5
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

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
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: A
* Processing Time per Message (ns/msg): 385.5
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 123.559
        * p90: 352.393
        * p99: 899.821
        * p99.9: 4450.35
        * max: 1.06647e+07
    * total (msgs): 117145568
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: F
* Processing Time per Message (ns/msg): 311.384
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 55.9618
        * p90: 273.714
        * p99: 846.63
        * p99.9: 5283.68
        * max: 554079
    * total (msgs): 1485888
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: E
* Processing Time per Message (ns/msg): 371.214
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 114.694
        * p90: 370.124
        * p99: 917.552
        * p99.9: 4166.66
        * max: 1.84284e+06
    * total (msgs): 5722824
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: C
* Processing Time per Message (ns/msg): 246.92
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 44.3262
        * p90: 269.282
        * p99: 820.034
        * p99.9: 2500
        * max: 438379
    * total (msgs): 99917
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: X
* Processing Time per Message (ns/msg): 257.984
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 52.0833
        * p90: 262.633
        * p99: 802.304
        * p99.9: 2012.41
        * max: 707731
    * total (msgs): 2787676
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: D
* Processing Time per Message (ns/msg): 373.428
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 145.168
        * p90: 330.23
        * p99: 882.091
        * p99.9: 4024.82
        * max: 3.28273e+07
    * total (msgs): 114360997
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: U
* Processing Time per Message (ns/msg): 597.877
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 231.604
        * p90: 598.403
        * p99: 1130.32
        * p99.9: 8758.85
        * max: 2.30042e+06
    * total (msgs): 21639067
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077
    
## Naming Convention used
* Files: snake_case (all lowercase with _ (underscores) as separators between words)
* Functions: PascalCase (Each word starts with an uppercase letter)
* Variables: snake_case
* Class: PascalCase

* c++ files: .cc
* header files: .h
* .inc?

* namespace: fh_lob
