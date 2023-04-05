#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "run_thread.hpp"

void benchmark_write_bw(int num_threads, int* access_size, uint8_t* start, bool read_bench, bool nt);

#endif
