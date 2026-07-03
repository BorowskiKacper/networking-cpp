# HFT Feed Handler and Limit Order Book (in progress)
## Most important offline Metrics:
* Throughput: Total Messages Processed / Total Time Taken
* Processsing Time per Message (The Offline Latency): Since I cannot measure network latency from my laptop, I'm measuring Internal processing latency.
* Maximum Latency (The Jitter Test): 99th and 99.9th percentile processing times.
* Cache Misses (The Diagnostic Metric): Mimimize cache misses as much as possible to maximize throughput.

## Current Scores:
* Throughput (msgs/sec): 4.11172e+06
    * Total Time (ns): 65360658015
    * Total Messages (msgs): 268744780
* Processing Time per Message (ns/msg): 410.44
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 140.181
        * p90: 401.152
        * p99: 988.473
        * p99.9: 6418.43
        * max: 1.35599e+07
    * total (msgs): 268734779
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077
ns_per_cycle: 0.554077
Again find ns_per_cycle: 0.554078

msg_type: S
* Processing Time per Message (ns/msg): 985175
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 56453.8
        * p90: 2.52368e+06
        * p99: 2.52368e+06
        * p99.9: 2.52368e+06
        * max: 2.52242e+06
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
* Processing Time per Message (ns/msg): 405.564
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 129.1
        * p90: 387.854
        * p99: 970.743
        * p99.9: 6489.35
        * max: 1.21208e+07
    * total (msgs): 117145568
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: F
* Processing Time per Message (ns/msg): 348.573
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 55.4077
        * p90: 290.336
        * p99: 890.956
        * p99.9: 7198.57
        * max: 1.05549e+06
    * total (msgs): 1485888
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: E
* Processing Time per Message (ns/msg): 393.077
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 121.343
        * p90: 410.017
        * p99: 979.608
        * p99.9: 5851.05
        * max: 6.57248e+06
    * total (msgs): 5722824
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: C
* Processing Time per Message (ns/msg): 278.55
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 47.0965
        * p90: 285.904
        * p99: 855.495
        * p99.9: 3351.06
        * max: 521644
    * total (msgs): 99917
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: X
* Processing Time per Message (ns/msg): 273.732
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 53.1914
        * p90: 275.93
        * p99: 837.764
        * p99.9: 3528.36
        * max: 4.84506e+06
    * total (msgs): 2787676
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: D
* Processing Time per Message (ns/msg): 396.378
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 151.817
        * p90: 365.691
        * p99: 935.282
        * p99.9: 5709.21
        * max: 1.35599e+07
    * total (msgs): 114360997
    * clipped (msgs): 0
    * ns_per_cycle (ns/cycle): 0.554077

msg_type: U
* Processing Time per Message (ns/msg): 636.31
* Maximum Latency:
    * Overall Histogram (ns/msg):
        * p50: 242.686
        * p90: 660.46
        * p99: 1250
        * p99.9: 10851
        * max: 3.7367e+06
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
