# Swanson's microbenchmarks

This is a set of microbenchmarks inpired by the paper [USENIX FAST paper](https://www.usenix.org/conference/fast20/presentation/yang).


## How to run

You can use the file "benchmark.cc" to take the measurements. Run the ./run_exp.sh script that measures sequential and random latencies and bandwidths of reads and writes. You can also run the ./benchmark executable using the required arguments.

The arguments are: <#threads> <0 bandwidth, 1 latency> <0 write, 1 read> <0 clwb, 1 non-temporal store> <0 sequential, 1 random>

To measure sequential latency of reads and writes: <br />
   ./benchmark 1 1 0 0 0 >> write_latency.txt <br />
   ./benchmark 1 1 1 0 0 >> read_latency.txt  <br />
   
To measure random latency of reads and writes: <br />
   ./benchmark 1 1 0 0 1 >> write_latency.txt <br />
   ./benchmark 1 1 1 0 1 >> read_latency.txt  <br />
   
To measure sequential bandwidth of reads and writes for 1 and 8 threads: <br />
   ./benchmark 1 0 0 0 0 >> results/write_bandwidth.txt  <br />
   ./benchmark 8 0 0 0 0 >> results/write_bandwidth.txt  <br />
   ./benchmark 1 0 1 0 0 >> results/read_bandwidth.txt   <br />
   ./benchmark 8 0 1 0 0 >> results/read_bandwidth.txt   <br />

To measure random bandwidth of reads and writes for 1 and 8 threads: <br />
   ./benchmark 1 0 0 0 1 >> results/write_bandwidth.txt <br />
   ./benchmark 8 0 0 0 1 >> results/write_bandwidth.txt <br />
   ./benchmark 1 0 1 0 1 >> results/read_bandwidth.txt <br />
   ./benchmark 8 0 1 0 1 >> results/read_bandwidth.txt <br />
   
   
   
   

