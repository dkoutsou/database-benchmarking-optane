// benchmarks for:
// 1. read/write latency
// 2. read/write bw

//#define _GNU_SOURCE
#include "run_thread.hpp"
#include <stdio.h>
#include <time.h>
#include <sched.h>  
#include <pthread.h>
#include <sys/time.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sys/mman.h>
//#include <linux/random.h>



using namespace std;

static void SFENCE() {
	asm volatile ("sfence" ::: "memory");
}

static void MFENCE() {
	asm volatile ("mfence" ::: "memory");
}


uint64_t(*latency_funcs_64byte[])(char *) = {
    &load_64byte_fence, // load + fence
    &load_64byte_fence_nt, // non-temporal load + fence
    &store_64byte_fence, // store + fence
    &store_64byte_clflush, // store + clflush
    &store_64byte_clwb, // store + clwb
    &store_64byte_clflushopt, // store + clflushopt
    &nstore_64byte_fence, // non-temporal store + fence
    &store_64byte_fence_movq, // store + fence (movq)
    &store_64byte_clflush_movq, // store - clflush (movq)
    &load_64byte_fence_movq, // load + fence (movq)
    &baseline
};

uint64_t(*latency_funcs_256byte[])(char *) = {
    &load_256byte_fence,
    &load_256byte_fence_nt,
    &store_256byte_fence,
    &store_256byte_clflush,
    &store_256byte_clwb,
    &store_256byte_clflushopt,
    &nstore_256byte_fence,
    &store_256byte_fence_movq,
    &store_256byte_clflush_movq,
    &load_256byte_fence_movq,
    &baseline
};


uint64_t(*latency_funcs_128byte[])(char *) = {
    &load_128byte_fence,
    &load_128byte_fence_nt,
    &store_128byte_fence,
    &store_128byte_clflush,
    &store_128byte_clwb,
    &store_128byte_clflushopt,
    &nstore_128byte_fence,
    &store_128byte_fence_movq,
    &store_128byte_clflush_movq,
    &load_128byte_fence_movq,
    &baseline
};

const char* latency_bench_map[] = {

	"load-fence",
	"ntload-fence",
	"store-fence",
	"store-clflush",
	"store-clwb",
	"store-clflushopt",
	"ntstore-fence",
	"store-fence-movq",
	"store-clflush-movq",
	"load-fence-movq",
	"baseline"
};

uint64_t (*bench_func[BASIC_OPS_TASK_COUNT])(char *) = {
    &load_64byte_fence,
    &store_64byte_clwb,
    &nstore_64byte_fence,
};


static bw_func funcs[] {
	&stride_load,
	&stride_nt,
	&stride_storeclwb,
	&stride_ssd_load,
	&stride_ssd_mmap_load,
	&stride_ssd_store,
	&stride_ssd_mmap_store_no_drain,
};


static bw_func_rand funcs_rand[] {
	&sizebw_load,
	&sizebw_nt,
	&sizebw_storeclwb,
};




void* random_lat(void* arg) {

	struct worker_ctx *ctx = (struct worker_ctx *)arg;
	static unsigned long flags;
	long access_size = ctx->access_size;
	long stride_size = access_size;
	long delay = 0;
	int op = ctx->op; // 0: ntstore, 1: store+clwb
	int tid = ctx->tid;
	char *buf = ctx->addr;
	char *region_end = buf + PERTHREAD_WORKSET;
	char* loc = ctx->loc;
	
        double time_passed;
	double total_time = 0.0;

	// set affinity
  	cpu_set_t cpuset; 
  	int cpu = ctx->core_id;
  	CPU_ZERO(&cpuset);       
  	CPU_SET(cpu,&cpuset); 
  	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);

	// latency test here
	int skip = 64;
	vector<uint64_t> rbuf(LATENCY_OPS_COUNT, 0.0); // TODO: check data type here

	long count = PERTHREAD_WORKSET/256;
	funcs[0](buf, 256, 256, 0, count); //warm up the system


	auto start = std::chrono::high_resolution_clock::now();

	for (int j = 0; j < LATENCY_OPS_COUNT; j++) {
		
	    uint64_t result = bench_func[op](&(buf[loc[j] & BASIC_OP_MASK]))/3.8 ; // TODO: cycles or ns?
		buf += skip;
		rbuf[j] = result;
	
	}

	auto end = std::chrono::high_resolution_clock::now();
	time_passed = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();

	// get average time + stdev
	//printf("Thread 0, Benchmark test 0 completed! It took %f ns!\n", time_passed);


	uint64_t total = 0;
	for (auto it: rbuf) {
		//std::cout << it << std::endl;
		total += it;
	}

	double average = (total*1.0)/LATENCY_OPS_COUNT;
	
	double var = 0.0;
	for(int i = 0; i < LATENCY_OPS_COUNT; i++ ) {
  		var += (rbuf[i] - average) * (rbuf[i] - average);
	}
	var /= LATENCY_OPS_COUNT;
	double sd = sqrt(var);

	printf("Latency test: Average: %lf ns, Stddev: %lf\n", average, sd);
	return 0;

}


//UPDATE!
void* random_ssd_lat(void* arg) {

    struct worker_ctx *ctx = (struct worker_ctx *)arg;
    static unsigned long flags;
    long access_size = ctx->access_size;
    long stride_size = access_size;
    long delay = 0;
    int op = ctx->op; // 0: read, 1: write
    int tid = ctx->tid;
    int fd = ctx->fd;
    int mmap = fd != 0 ? 0 : 1;
    char *buf = ctx->addr;
    char* loc = ctx->loc;

    double time_passed;
    double total_time = 0.0;

    // set affinity
    cpu_set_t cpuset;
    int cpu = ctx->core_id;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu,&cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);

    // latency test here
    int skip = 4096;
    vector<uint64_t> rbuf(LATENCY_OPS_COUNT, 0.0); // TODO: check data type here

    long count = PERTHREAD_WORKSET/4096;
    //funcs[3](buf, access_size, access_size, (long)fd, count);

    auto start = std::chrono::high_resolution_clock::now();

    for (int j = 0; j < LATENCY_OPS_COUNT; j++) {

        // uint64_t result = funcs[op + mmap + 3](buf, access_size, access_size, (long)fd, count);
        // buf += skip;
        // rbuf[j] = result;

    }

    auto end = std::chrono::high_resolution_clock::now();
    time_passed = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();

    // get average time + stdev
    //printf("Thread 0, Benchmark test 0 completed! It took %f ns!\n", time_passed);


    uint64_t total = 0;
    for (auto it: rbuf) {
        //std::cout << it << std::endl;
        // total += it;
    }

    double average = (total*1.0)/LATENCY_OPS_COUNT;

    double var = 0.0;
    for(int i = 0; i < LATENCY_OPS_COUNT; i++ ) {
        // var += (rbuf[i] - average) * (rbuf[i] - average);
    }
    // var /= LATENCY_OPS_COUNT;
    // double sd = sqrt(var);

    // printf("Latency test: Average: %lf ns, Stddev: %lf\n", average, sd);
    return 0;

}

void* seq_lat(void *arg) {

	struct worker_ctx *ctx = (struct worker_ctx *)arg;
	static unsigned long flags;
	long access_size = ctx->access_size;
	long stride_size = access_size;
	long delay = 0;
	int op = ctx->op; // 0: ntstore, 1: store+clwb
	int tid = ctx->tid;
	char *buf = ctx->addr;
	char *region_end = buf + PERTHREAD_WORKSET;

        double time_passed;
	double total_time = 0.0;

	// set affinity
  	cpu_set_t cpuset; 
  	int cpu = ctx->core_id;
  	CPU_ZERO(&cpuset);       
  	CPU_SET(cpu,&cpuset); 
  	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);

	// latency test here
	int skip = 64;
	vector<double> rbuf(LATENCY_OPS_COUNT, 0.0); // TODO: check data type here

	long count = PERTHREAD_WORKSET/256;
	funcs[0](buf, 256, 256, 0, count);

	auto start = std::chrono::high_resolution_clock::now();


	for (int j=0; j<LATENCY_OPS_COUNT; j++) {
		
		double result = bench_func[op](buf)/3.8; // TODO: cycles or ns?
		buf += skip;
		rbuf[j] = result;
	
	}

	auto end = std::chrono::high_resolution_clock::now();
	time_passed = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();

	// get average time + stdev
	//printf("Thread 0, Benchmark test 0 completed! It took %f ns!, Average is %f ns\n", time_passed, time_passed/LATENCY_OPS_COUNT);


	double total = 0.0;
	for (auto it: rbuf) {
		//std::cout << it << std::endl;
		total += it;
	}

	double average = (total*1.0)/LATENCY_OPS_COUNT;
	
	double var = 0.0;
	for(int i = 0; i < LATENCY_OPS_COUNT; i++ ) {
  		var += (rbuf[i] - average) * (rbuf[i] - average);
	}
	var /= 10*LATENCY_OPS_COUNT;
	double sd = sqrt(var);

	printf("Latency test: Average: %lf ns, Stddev: %lf\n", average, sd);
	return 0;
}


void* strided_bwjob(void *arg)
{
	
	struct worker_ctx *ctx = (struct worker_ctx *)arg;
	static unsigned long flags;
	long access_size = ctx->access_size;
	long stride_size = access_size;
	long delay = 0;
	int op = ctx->op; // 0: ntstore, 1: store+clwb
	int tid = ctx->tid;
	char *buf = ctx->addr;
	char *region_end = buf + PERTHREAD_WORKSET;
	clock_t start, end;
    	double time_passed;
	double total_time = 0.0;

	// set affinity
  	cpu_set_t cpuset; 
  	int cpu = ctx->core_id;
  	CPU_ZERO(&cpuset);       
  	CPU_SET(cpu,&cpuset); 
  	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
	//sched_setaffinity(0, sizeof(cpuset), &cpuset);


	// TODO: count?
	long count = PERTHREAD_WORKSET / access_size;

	//if (stride_size * count > PERTHREAD_WORKSET)
	//	count = PERTHREAD_WORKSET / stride_size;

	//printf("About to call lfs_stride_bw, buf is %p, end is %p,  op is: %d, access_size is %ld, stride_size is %ld, delay is %ld, count is %ld\n",
	//			buf, region_end, op, access_size, stride_size, delay, count);
	

	// WARMUP
	funcs[op](buf, access_size, stride_size, 0, count);
	MFENCE();

	for (int i=0; i<10; i++) {
		
		auto start = std::chrono::high_resolution_clock::now();

		for (int k=0; k<10; k++) {
			//printf("Test %d, Buf is: %p\n", i, buf);
			funcs[op](buf, access_size, stride_size, 0, count);

			if (op==0)
				MFENCE();
			else
				SFENCE();
		}

		auto end = std::chrono::high_resolution_clock::now();
		time_passed = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();

		total_time += time_passed/1000000;
		//printf("Thread %d, Benchmark test %d completed! It took %f seconds!\n", tid, i, time_passed/1000000);
	}


	//printf("Thread %d, time: %f seconds\n", tid, total_time/10);

	
	return 0;
}

inline int rand_r_32 (unsigned int *seed)
{
    unsigned int next = *seed;
    int result;

    next *= 1103515245;
    next += 12345;
    result = (unsigned int) (next / 65536) % 2048;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    *seed = (unsigned int)next;

    return result;
}




void* size_bwjob(void *arg) 
{

    struct worker_ctx *ctx = (struct worker_ctx *)arg;
    static unsigned long flags;
    long access_size = ctx->access_size;
    long stride_size = access_size;
    long delay = 0;
    int op = ctx->op; // 0: ntstore, 1: store+clwb
    int tid = ctx->tid;
    char *buf = ctx->addr;
    char *region_end = buf + PERTHREAD_WORKSET;
    clock_t start, end;
    double time_passed;
    double total_time = 0.0;
    long align_size = 1 << 6; //cacheline alighnment
    long bitmask = (PERTHREAD_WORKSET - 1) ^ (align_size - 1);
    unsigned int seed;
    //get_random_bytes(&seed, sizeof(unsigned long));
    rand_r_32(&seed);

    // set affinity
    cpu_set_t cpuset; 
    int cpu = ctx->core_id;
    CPU_ZERO(&cpuset);       
    CPU_SET(cpu,&cpuset); 
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    //sched_setaffinity(0, sizeof(cpuset), &cpuset)

    long count = PERTHREAD_WORKSET / access_size;

    // WARMUP
    funcs[op](buf, access_size, stride_size, 0, count);
    MFENCE();

    for (int i=0; i<10; i++) {
    //auto start = std::chrono::high_resolution_clock::now();
	for (int k=0; k<10; k++) {
	    funcs_rand[op](buf, access_size, count, &seed, bitmask);
	        if (op==0)
		    MFENCE();
	        else
		    SFENCE();
	}

	//auto end = std::chrono::high_resolution_clock::now();
	//time_passed = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
	//total_time += time_passed/1000000;
	//printf("Thread %d, Benchmark test %d completed! It took %f seconds!\n", tid, i, time_passed/1000000);
    }
    //printf("Thread %d, time: %f seconds\n", tid, total_time/10);	

    return 0;

}







void* strided_ssd_bwjob(void *arg) {

    struct worker_ctx *ctx = (struct worker_ctx *)arg;
    static unsigned long flags;
    long access_size = ctx->access_size;
    long stride_size = access_size;
    long delay = 0;
    long fd = ctx->fd;
    int mmap = (fd != 0) ? 0 : 1;

    int op = ctx->op + mmap + 3; // 3: read, 4: read-mmap, 5: write, 6: write-mmap
    int tid = ctx->tid;
    long offset = tid * PERTHREAD_WORKSET;
    //std::cout << offset << std::endl;
    char *buf = ctx->addr;
    clock_t start, end;
    double time_passed;
    double total_time = 0.0;

    // set affinity
    cpu_set_t cpuset;
    int cpu = ctx->core_id;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu,&cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    //sched_setaffinity(0, sizeof(cpuset), &cpuset);

    // TODO: count?
    long count = PERTHREAD_WORKSET / access_size;

    // WARMUP
    funcs[op](buf, access_size, offset, fd, count);
    // fence?

    for (int i = 0; i < 10; i++) {

        auto start = std::chrono::high_resolution_clock::now();

        for (int k = 0; k < 10; k++) {
            //printf("Test %d, Buf is: %p\n", i, buf);
            funcs[op](buf, access_size, offset, fd, count);

            if (mmap == 1) {
		int res = msync(buf, PERTHREAD_WORKSET, MS_SYNC);
		if (res == -1) {
		    perror("msync");
		    exit(1);
		}
	    } else {
		int res = fsync(fd);
	        if (res == -1) {
		    perror("fsync");
		    exit(1);
		}
	    }

            auto end = std::chrono::high_resolution_clock::now();
            time_passed = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();

            total_time += time_passed/1000000;
            //printf("Thread %d, Benchmark test %d completed! It took %f seconds!\n", tid, i, time_passed/1000000);
        }
    }

    //printf("Thread %d, time: %f seconds\n", tid, total_time/10);
    return 0;
}





