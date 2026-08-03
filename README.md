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


## Naming Convention used
* Files: snake_case (all lowercase with _ (underscores) as separators between words)
* Functions: PascalCase (Each word starts with an uppercase letter)
* Variables: snake_case
* Class: PascalCase

* c++ files: .cc
* header files: .h
* .inc?

* namespace: fh_lob
