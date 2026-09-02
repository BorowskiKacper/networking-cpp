# HFT Feed Handler and Limit Order Book (in progress)
## Most important offline Metrics:
* Throughput: Total Messages Processed / Total Time Taken
* Processsing Time per Message (The Offline Latency): Since I cannot measure network latency from my laptop, I'm measuring Internal processing latency.
* Maximum Latency (The Jitter Test): 99th and 99.9th percentile processing times.
* Cache Misses (The Diagnostic Metric): Mimimize cache misses as much as possible to maximize throughput.

<!-- OFFLINE METRICS_START -->
* Throughput (msgs/sec): 7.25701e+06
    * Total Time (ns): 37032457586
    * Total Messages (msgs): 268744780
* Processing Time per Message (ns/msg): 211.595
* total (msgs): 268734779
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 68.1516
    * p90: 233.821
    * p99: 757.979
    * p99.9: 882.092
    * max: 213109
    * clipped (msgs): 0
ns_per_cycle: 0.554078
Again find ns_per_cycle: 0.554078

msg_type: S
* Processing Time per Message (ns/msg): 25443.4
* total (msgs): 5
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 14255.3
    * p90: 15531.9
    * p99: 15531.9
    * p99.9: 15531.9
    * max: 15518.6
    * clipped (msgs): 0

msg_type: R
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

msg_type: A
* Processing Time per Message (ns/msg): 188.879
* total (msgs): 117145568
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 61.5027
    * p90: 198.36
    * p99: 722.518
    * p99.9: 837.766
    * max: 213109
    * clipped (msgs): 0

msg_type: F
* Processing Time per Message (ns/msg): 158.897
* total (msgs): 1485888
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 41.0018
    * p90: 178.413
    * p99: 695.922
    * p99.9: 1391.84
    * max: 50963.5
    * clipped (msgs): 0

msg_type: E
* Processing Time per Message (ns/msg): 230.48
* total (msgs): 5722824
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 89.2066
    * p90: 260.417
    * p99: 793.44
    * p99.9: 917.553
    * max: 38950.6
    * clipped (msgs): 0

msg_type: C
* Processing Time per Message (ns/msg): 142.351
* total (msgs): 99917
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 33.2447
    * p90: 187.278
    * p99: 687.057
    * p99.9: 864.362
    * max: 15272.1
    * clipped (msgs): 0

msg_type: X
* Processing Time per Message (ns/msg): 140.312
* total (msgs): 2787676
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 32.6906
    * p90: 182.846
    * p99: 687.057
    * p99.9: 864.362
    * max: 122504
    * clipped (msgs): 0

msg_type: D
* Processing Time per Message (ns/msg): 228.554
* total (msgs): 114360997
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 85.8821
    * p90: 249.335
    * p99: 784.574
    * p99.9: 890.957
    * max: 133378
    * clipped (msgs): 0

msg_type: U
* Processing Time per Message (ns/msg): 302.559
* total (msgs): 21639067
* ns_per_cycle (ns/cycle): 0.554078
* HDR Histogram (ns/msg): 
    * p50: 131.316
    * p90: 275.931
    * p99: 820.035
    * p99.9: 953.014
    * max: 64279.1
    * clipped (msgs): 0
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
