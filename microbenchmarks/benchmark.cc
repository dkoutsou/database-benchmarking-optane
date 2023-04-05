#include <stdlib.h>
#include <stdio.h>
#include "benchmark.hpp"
#include <malloc.h>
#include <pthread.h>
#include <libpmem.h>
#include <assert.h>
#include <numa.h>
#include <time.h>
#include <chrono>
#include <sys/random.h>
#include <fcntl.h>
#include <sys/mman.h>

#define REGION_SIZE 58719476736ULL
#define PR_FILE_NAME "/mnt/pmem0/file2"
#define PR_SSD_FILE_NAME "/home/mkorenberg/ml-nvm/custom/ssd_file_2"
#define ALIGNMENT 4096ULL
#define BIT_SHIFT 12
#define ONE_GB 1024*1024*1024

static uint8_t *PMEM_ADDR;
static uint8_t *PMEM_SSD_ADDR;
static uint8_t *DRAM_ADDR;

int affinity[48] = {0,2,4,6,8,10,12,14,
		   16,18,20,22,24,26,28,
		   30,32,34,36,38,40,42,
		   44,46,48,50,52,54,56,
		   58,60,62,64,66,68,70,
		   72,74,76,78,80,82,84,
		   86,88,90,92,94};


uint8_t* allocated_buf;

//==============================================MAPPING================================================

static void mapPersistentRegion(const char* filename, uint8_t* regionAddr, const uint64_t regionSize) {

    size_t mapped_len;
    int is_pmem;
	
    if ((PMEM_ADDR = (uint8_t*)pmem_map_file(filename, regionSize, PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem)) == nullptr) {
    	perror("pmem_map_file");
        exit(1);
    }
    assert (is_pmem > 0);
}


int mapPersistentSSDRegion(const char* filename, uint8_t* regionAddr, const uint64_t regionSize, bool mmaped) {

    mode_t mode = (mode_t)0666;
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, mode);
    if (fd == -1) {
	perror("open");
	exit(1);
    }

    int res = ftruncate(fd, regionSize);
    if (res == -1) {
	perror("ftruncate");
	exit(1);
    }

    if (mmaped) {
        void* addr;
        if ((addr = mmap(nullptr, (size_t)regionSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
    	    perror("mmap");
            exit(1);
        }
        PMEM_SSD_ADDR = (uint8_t*)addr;
    }
    return fd;
}

//============================================LATENCY=====================================================

void benchmark_lat_seq(uint8_t* start, bool read_bench, bool nt) {
    
    pthread_t thread;
    struct worker_ctx* ctx = (struct worker_ctx*)malloc(sizeof(struct worker_ctx));

    ctx->addr = (char*)start;

    if (read_bench)
        ctx->op = 0;  // read
    else if (nt)
	ctx->op = 1; // write-nt
    else 
        ctx->op = 2; // write-store+clwb

    ctx->core_id = affinity[0];
    ctx->tid = 0;

    printf("About to start latency test, for thread 0, op is %d\n", ctx->op); 
    pthread_create(&thread, NULL, seq_lat, ctx);

    pthread_join(thread, NULL);

}

void benchmark_lat_random(uint8_t* start, bool read_bench, bool nt) {
    
    pthread_t thread;
    struct worker_ctx* ctx = (struct worker_ctx*)malloc(sizeof(struct worker_ctx));

    // TODO: loc: random bytes
    char* loc = (char*)numa_alloc_onnode(LATENCY_OPS_COUNT*8, 0);
    bool b = getrandom(loc, LATENCY_OPS_COUNT * 8, GRND_RANDOM);

    ctx->addr = (char*)start;
    ctx->loc = loc;

    if (read_bench)
        ctx->op = 0;  // read
    else if (nt)
	ctx->op = 1; // write-nt
    else 
        ctx->op = 2; // write-store+clwb

    ctx->core_id = affinity[0];
    ctx->tid = 0;

    printf("About to start latency test, for thread 0, op is %d\n", ctx->op); 
    pthread_create(&thread, nullptr, random_lat, ctx);

    pthread_join(thread, NULL);

}


void benchmark_lat_random_ssd(uint8_t* start, bool read_bench, bool mmap, int fd) {

    pthread_t thread;
    struct worker_ctx* ctx = (struct worker_ctx*)malloc(sizeof(struct worker_ctx));

    // TODO: loc: random bytes
    char* loc = (char*)numa_alloc_onnode(LATENCY_OPS_COUNT*8, 0);
    bool b = getrandom(loc, LATENCY_OPS_COUNT * 8, GRND_RANDOM);

    ctx->addr = (char*)start;
    ctx->loc = loc;
    if (mmap) {
        ctx->fd = 0;
    } else {
	ctx->fd = 1;
    }

    if (read_bench) {
        ctx->op = 0;  // read
    } else {
        ctx->op = 2;  // write
    }
  
    ctx->core_id = affinity[0];
    ctx->tid = 0;

    printf("About to start latency test, for thread 0, op is %d\n", ctx->op);
    pthread_create(&thread, nullptr, random_ssd_lat, ctx);

    pthread_join(thread, nullptr);

}

//==============================================BANDWIDTH==================================================
// given: access size, num of threads
// measures both read and write bandwidth
void benchmark_bw_seq(int num_threads, int* access_size, uint8_t* start, bool read_bench, bool nt, bool dram) {

    pthread_t thread_arr[num_threads];
   
    auto begin = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; i++) {    
        struct worker_ctx* ctx = (struct worker_ctx*)malloc(sizeof(struct worker_ctx));

        //uint8_t* addr = (uint8_t*)memalign(64, PERTHREAD_WORKSET);
        uint8_t* addr = start + (i * PERTHREAD_WORKSET); // TODO: check for false sharing here
        ctx->addr = (char*)addr;
        ctx->access_size = access_size[0];

        if (read_bench)
            ctx->op = 0;  // read
        else if (nt)
	    ctx->op = 1; // write-nt
        else 
            ctx->op = 2; // write-store+clwb

        ctx->core_id = affinity[i];
        ctx->tid = i;

        //printf("About to start bandwidth test, for thread %d, op is %d\n", i, ctx->op); 
        pthread_create(&thread_arr[i], NULL, strided_bwjob, ctx);
    }

    for (int i=0; i<num_threads; i++) 
        pthread_join(thread_arr[i], NULL);

    auto end = std::chrono::high_resolution_clock::now();
    float duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
    duration /= 1000;

    float bw = (num_threads*100*2)/duration; // each thread writes 2GB
    printf("%d dev, %d threads, %f duration, %f GB/sec\n", int(dram), num_threads, duration, bw);
    //printf("------------------------- TOTAL TIME for %d threads is: %f seconds, bandwidth is %f GB/sec\n", num_threads, duration, bw);

}


// given: access size, num of threads
// measures both read and write bandwidth
void benchmark_bw_random(int num_threads, int* access_size, uint8_t* start, bool read_bench, bool nt, bool dram) {

    pthread_t thread_arr[num_threads];

    auto begin = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; i++) {
        struct worker_ctx* ctx = (struct worker_ctx*)malloc(sizeof(struct worker_ctx));

        //uint8_t* addr = (uint8_t*)memalign(64, PERTHREAD_WORKSET);
        uint8_t* addr = start + (i * PERTHREAD_WORKSET); // TODO: check for false sharing here
        ctx->addr = (char*)addr;
        ctx->access_size = access_size[0];

        if (read_bench)
            ctx->op = 0;  // read
        else if (nt)
            ctx->op = 1; // write-nt
        else
            ctx->op = 2; // write-store+clwb

        ctx->core_id = affinity[i];
        ctx->tid = i;

        //printf("About to start bandwidth test, for thread %d, op is %d\n", i, ctx->op); 
        pthread_create(&thread_arr[i], NULL, size_bwjob, ctx);
    }

    for (int i=0; i<num_threads; i++)
        pthread_join(thread_arr[i], NULL);

    auto end = std::chrono::high_resolution_clock::now();
    float duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
    duration /= 1000;

    float bw = (num_threads*100*2)/duration; // each thread writes 2GB
    printf("%d dev, %d threads, %f duration, %f GB/sec\n", int(dram), num_threads, duration, bw);
    //printf("------------------------- TOTAL TIME for %d threads is: %f seconds, bandwidth is %f GB/sec\n", num_threads, duration, bw);

}








// given: access size, num of threads
// measures both read and write bandwidth
void benchmark_bw_ssd(int num_threads, int* access_size, uint8_t* start, bool read_bench, bool mmap, int fd) {

    pthread_t thread_arr[num_threads];

    auto begin = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; i++) {
        struct worker_ctx* ctx = (struct worker_ctx*)malloc(sizeof(struct worker_ctx));

        //uint8_t* addr = (uint8_t*)memalign(64, PERTHREAD_WORKSET);
        uint8_t* addr;
	if (mmap) {
	    addr = start + (i * PERTHREAD_WORKSET); // TODO: check for false sharing here
	    ctx->fd = 0;
	} else {
	    addr = nullptr;
	    ctx->fd = fd;
	}
	ctx->addr = (char*)addr;
        ctx->access_size = access_size[0];

        if (read_bench)
            ctx->op = 0;  // read
        else
            ctx->op = 2;  // write

        ctx->core_id = affinity[i];
        ctx->tid = i;

        //printf("About to start bandwidth test, for thread %d, op is %d\n", i, ctx->op); 
        pthread_create(&thread_arr[i], nullptr, strided_ssd_bwjob, ctx);
    }

    for (int i = 0; i < num_threads; i++)
        pthread_join(thread_arr[i], nullptr);

    auto end = std::chrono::high_resolution_clock::now();
    float duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
    duration /= 1000;

    float bw = (num_threads*100*2)/duration; // each thread writes 2GB
    printf("%d dev, %d threads, %f duration, %f GB/sec\n", 0, num_threads, duration, bw);
    //printf("------------------------- TOTAL TIME for %d threads is: %f seconds, bandwidth is %f GB/sec\n", num_threads, duration, bw);

}


int main(int argc, char** argv) {

    if (argc < 5) {
        printf("Usage: ./benchmark <num_threads> <bw (0) / lat (1)> <read (1), write (0)> <ntstore (1), clwb (0)> <random (1), seq (0)>\n");
        exit(1);
    }

    int num_threads = atoi(argv[1]);
    int bench = atoi(argv[2]);
    bool toread = (atoi(argv[3])==1);
    bool ntstore = (atoi(argv[4])==1);
    bool mmap = ntstore; ////for SSD if ntstore == 1 we use mmap
    bool random = (atoi(argv[5])==1);

    allocated_buf = new uint8_t[PERTHREAD_WORKSET];

    printf("Start benchmarking with %d threads\n", num_threads);
    
    if (bench == 0)
	    printf("Measure bandwidth\n");
    else
	    printf("Measure latency\n");

    if (toread)
	    printf("Read accesses\n");
    else
	    printf("Write accesses, ntstore/mmap is %d\n", atoi(argv[4]));

    if (random)
            printf("Random accesses\n");
    else
            printf("Sequential accesses\n");

    int access_sizes[2] = {256, 4096};
    
    // map NVM and allocate DRAM
    mapPersistentRegion(PR_FILE_NAME, nullptr, REGION_SIZE);

    unsigned long long sz = REGION_SIZE + ALIGNMENT;
    uint8_t* addr = (uint8_t*)numa_alloc_onnode(sz, 0);
    DRAM_ADDR = (uint8_t*)((size_t)(addr + ALIGNMENT-1) & ~(ALIGNMENT-1));
    memset(DRAM_ADDR, 0x01, REGION_SIZE);

    if (!random) {
	printf("----------------------- Run seq benchmark on DRAM --------------------\n");
        if (bench == 0)
    	    benchmark_bw_seq(num_threads, access_sizes, DRAM_ADDR, toread, ntstore, true);
        else 
	    benchmark_lat_seq(DRAM_ADDR, toread, ntstore);
    } else {
	printf("----------------------- Run rand benchmark on DRAM --------------------\n");
        if (bench == 0)
            benchmark_bw_random(num_threads, access_sizes, DRAM_ADDR, toread, ntstore, true);
        else
            benchmark_lat_random(DRAM_ADDR, toread, ntstore);
    }

    if (!random) {
	printf("----------------------- Run seq benchmark on PMEM --------------------\n");
        if (bench == 0)
     	    benchmark_bw_seq(num_threads, access_sizes, PMEM_ADDR, toread, ntstore, false);
        else
	    benchmark_lat_seq(PMEM_ADDR, toread, ntstore);
    } else {
        printf("----------------------- Run rand benchmark on PMEM --------------------\n");
        if (bench == 0)
            benchmark_bw_random(num_threads, access_sizes, PMEM_ADDR, toread, ntstore, false);
        else
            benchmark_lat_random(PMEM_ADDR, toread, ntstore);
    }


    delete [] allocated_buf;
    return 0;

}
