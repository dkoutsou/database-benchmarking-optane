#ifndef RUNTHREAD_H
#define RUNTHREAD_H

#include <unistd.h>
#include "utils.hpp"
#include "memaccess.hpp"
#include "common.hpp"

typedef void (*bw_func)(char* start_addr, long size, long skip, long delay, long count);
typedef void (*bw_func_rand)(char* start_addr, long size, long count, unsigned int *rand_seed, long access_mask);

void* strided_bwjob(void *arg);
void* size_bwjob(void *arg);
void* strided_ssd_bwjob(void *arg);
void* seq_lat(void *arg);
void* random_lat(void* arg);
void* random_ssd_lat(void* arg);

#endif
