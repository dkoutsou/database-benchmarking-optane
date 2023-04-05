/* SPDX-License-Identifier: GPL-2.0 */

// have removed kernel related instructions

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include "memaccess.hpp"
#include <linux/string.h>
#include <iostream>
#include <libpmem.h>
#include "arrayalloc.h"

extern uint32_t *lfs_random_array;

/* Arbitrary sizes w/o clearing pipeline */

// rax(ret), rcx(4), rdx(3), rsi(2), rdi(1), r8(5), r9(6), r10, r11 usable
// r12, r13, r14, r15, rbx, rsp, rbp are callee-saved (need push/pop)


#define SIZEBTNT_MACRO SIZEBTNT_512_AVX512
#define SIZEBTST_MACRO SIZEBTST_512_AVX512
#define SIZEBTLD_MACRO SIZEBTLD_512_AVX512
#define SIZEBTSTFLUSH_MACRO SIZEBTSTFLUSH_512_AVX512

//#define SIZEBTST_FENCE	"mfence \n"
//#define SIZEBTLD_FENCE	"mfence \n"
#define SIZEBTST_FENCE	""
#define SIZEBTLD_FENCE	""

#define CACHEFENCE_FENCE	"sfence \n"
//#define CACHEFENCE_FENCE	"mfence \n"


#define RandLFSR64 "mov    (%[random]), %%r9 \n"  \
				   "mov    %%r9, %%r12 \n"        \
				   "shr    %%r9 \n"               \
				   "and    $0x1, %%r12d \n"       \
				   "neg    %%r12 \n"              \
				   "and    %%rcx, %%r12 \n"       \
				   "xor    %%r9, %%r12 \n"        \
				   "mov    %%r12, (%[random]) \n" \
				   "mov    %%r12, %%r8 \n"        \
				   "and    %[accessmask], %%r8 \n"

void sizebw_load(char *start_addr, long size, long count, unsigned int *rand_seed, long access_mask)
{
	// ntstore 512B untl access size for count times
	// what is the role of "RandLFSR64"?
	// why 4?
	
	asm volatile (
		"movabs $0xd800000000000000, %%rcx \n"	/* rcx: bitmask used in LFSR */
		"xor %%r8, %%r8 \n"						/* r8: access offset */
		"xor %%r11, %%r11 \n"					/* r11: access counter */
// 1
"LOOP_FRNG_SIZEBWL_RLOOP: \n"					/* outer (counter) loop */
		RandLFSR64								/* LFSR: uses r9, r12 (reusable), rcx (above), fill r8 */
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"					/* r10: accessed size */
"LOOP_FRNG_SIZEBWL_ONE1: \n"					/* inner (access) loop, unroll 8 times */
		SIZEBTLD_MACRO							/* Access: uses r8[rand_base], r10[size_accessed], r9 */
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWL_ONE1 \n"
		SIZEBTLD_FENCE

// 2
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWL_ONE2: \n"
		SIZEBTLD_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWL_ONE2 \n"
		SIZEBTLD_FENCE
// 3
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWL_ONE3: \n"
		SIZEBTLD_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWL_ONE3 \n"
		SIZEBTLD_FENCE
// 4
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWL_ONE4: \n"
		SIZEBTLD_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWL_ONE4 \n"
		SIZEBTLD_FENCE

		"add $4, %%r11 \n"
		"cmp %[count], %%r11\n"
		"jl LOOP_FRNG_SIZEBWL_RLOOP \n"

		: [random]"=r"(rand_seed)
		: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), "0"(rand_seed), [accessmask]"r"(access_mask):
			"%rcx", "%r12", "%r11", "%r10", "%r9", "%r8");
}

void sizebw_nt(char *start_addr, long size, long count, unsigned int *rand_seed, long access_mask)
{

	// ntstore 512B untl access size for count times
	// what is the role of "RandLFSR64"?
	// why 4?

	asm volatile (
		"movabs $0xd800000000000000, %%rcx \n"
		"xor %%r11, %%r11 \n"
		"movq %[start_addr], %%xmm0 \n"			/* zmm0: read/write register */
// 1
"LOOP_FRNG_SIZEBWNT_RLOOP: \n"
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWNT_ONE1: \n"
		SIZEBTNT_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWNT_ONE1 \n"
		SIZEBTST_FENCE

// 2
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWNT_ONE2: \n"
		SIZEBTNT_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWNT_ONE2 \n"
		SIZEBTST_FENCE
// 3
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWNT_ONE3: \n"
		SIZEBTNT_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWNT_ONE3 \n"
		SIZEBTST_FENCE
// 4
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWNT_ONE4: \n"
		SIZEBTNT_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWNT_ONE4 \n"
		SIZEBTST_FENCE

		"add $4, %%r11 \n"
		"cmp %[count], %%r11\n"
		"jl LOOP_FRNG_SIZEBWNT_RLOOP \n"

		: [random]"=r"(rand_seed)
		: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), "0"(rand_seed), [accessmask]"r"(access_mask):
			"%rcx", "%r12", "%r11", "%r10", "%r9", "%r8");
}

void sizebw_store(char *start_addr, long size, long count, unsigned int *rand_seed, long access_mask)
{

	// store 512B untl access size for count times
	// what is the role of "RandLFSR64"?
	// why 4?

	asm volatile (
		"movabs $0xd800000000000000, %%rcx \n"
		"xor %%r11, %%r11 \n"
		"movq %[start_addr], %%xmm0 \n"			/* zmm0: read/write register */
// 1
"LOOP_FRNG_SIZEBWST_RLOOP: \n"
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWST_ONE1: \n"
		SIZEBTST_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWST_ONE1 \n"
		SIZEBTST_FENCE

// 2
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWST_ONE2: \n"
		SIZEBTST_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWST_ONE2 \n"
		SIZEBTST_FENCE
// 3
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWST_ONE3: \n"
		SIZEBTST_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWST_ONE3 \n"
		SIZEBTST_FENCE
// 4
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWST_ONE4: \n"
		SIZEBTST_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWST_ONE4 \n"
		SIZEBTST_FENCE

		"add $4, %%r11 \n"
		"cmp %[count], %%r11\n"
		"jl LOOP_FRNG_SIZEBWST_RLOOP \n"

		: [random]"=r"(rand_seed)
		: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), "0"(rand_seed), [accessmask]"r"(access_mask):
			"%rcx", "%r12", "%r11", "%r10", "%r9", "%r8");
}

void sizebw_storeclwb(char *start_addr, long size, long count, unsigned int *rand_seed, long access_mask)
{

	// store 512B untl access size for count times
	// do a clwb per cache line
	// what is the role of "RandLFSR64"?
	// why 4?
	asm volatile (
		"movabs $0xd800000000000000, %%rcx \n"
		"xor %%r11, %%r11 \n"
		"movq %[start_addr], %%xmm0 \n"			/* zmm0: read/write register */
// 1
"LOOP_FRNG_SIZEBWSTFLUSH_RLOOP: \n"
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWSTFLUSH_ONE1: \n"
		SIZEBTSTFLUSH_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWSTFLUSH_ONE1 \n"
		SIZEBTST_FENCE

// 2
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWSTFLUSH_ONE2: \n"
		SIZEBTSTFLUSH_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWSTFLUSH_ONE2 \n"
		SIZEBTST_FENCE
// 3
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWSTFLUSH_ONE3: \n"
		SIZEBTSTFLUSH_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWSTFLUSH_ONE3 \n"
		SIZEBTST_FENCE
// 4
		RandLFSR64
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"
"LOOP_FRNG_SIZEBWSTFLUSH_ONE4: \n"
		SIZEBTSTFLUSH_MACRO
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_FRNG_SIZEBWSTFLUSH_ONE4 \n"
		SIZEBTST_FENCE

		"add $4, %%r11 \n"
		"cmp %[count], %%r11\n"
		"jl LOOP_FRNG_SIZEBWSTFLUSH_RLOOP \n"

		: [random]"=r"(rand_seed)
		: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), "0"(rand_seed), [accessmask]"r"(access_mask):
			"%rcx", "%r12", "%r11", "%r10", "%r9", "%r8");
}

void stride_load(char *start_addr, long size, long skip, long delay, long count)
{

	// load "access_size" bytes starting from start_addr per 64B - use vmovntdqa
	// wait for "delay" cyscles
	// then add "skip bytes" to start_addr
	// continue for "count" iterations

	asm volatile (
		"xor %%r8, %%r8 \n"						/* r8: access offset */
		"xor %%r11, %%r11 \n"					/* r11: counter */

// 1
"LOOP_STRIDE_LOAD_OUTER: \n"						/* outer (counter) loop */
		"lea (%[start_addr], %%r8), %%r9 \n"	/* r9: access loc */
		"xor %%r10, %%r10 \n"					/* r10: accessed size */
"LOOP_STRIDE_LOAD_INNER: \n"						/* inner (access) loop, unroll 8 times */
		SIZEBTLD_256_AVX512						/* Access: uses r10[size_accessed], r9 */
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_STRIDE_LOAD_INNER \n"
		SIZEBTLD_FENCE // this is not used

		"xor %%r10, %%r10 \n"
"LOOP_STRIDE_LOAD_DELAY: \n"						/* delay <delay> cycles */
		"inc %%r10 \n"
		"cmp %[delay], %%r10 \n"
		"jl LOOP_STRIDE_LOAD_DELAY \n"

		"add %[skip], %%r8 \n"
		"inc %%r11 \n"
		"cmp %[count], %%r11 \n"

		"jl LOOP_STRIDE_LOAD_OUTER \n"

		:: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), [skip]"r"(skip), [delay]"r"(delay):
			"%r11", "%r10", "%r9", "%r8");
}


void stride_ssd_load(char *start_addr, long size, long offset, long fd, long count) {
    size_t total = (size_t)(count * size);
    //char *buf = new char[total];
    ssize_t result = pread(fd, allocated_buf, total, offset);
    if (result == -1) {
        perror("pread");
        exit(1);
    }
    //delete [] buf;
}


void stride_ssd_mmap_load(char *start_addr, long size, long offset, long fd, long count) {
    // load "access_size" bytes starting from start_addr per 64B - use vmovntdqa
    // wait for "delay" cyscles
    // then add "skip bytes" to start_addr
    // continue for "count" iterations
   long skip = size;
   long delay = 0;

        asm volatile (
                "xor %%r8, %%r8 \n"                                             /* r8: access offset */
                "xor %%r11, %%r11 \n"                                   /* r11: counter */

// 1
"LOOP_STRIDELOAD_MMAP_OUTER: \n"                                             /* outer (counter) loop */
                "lea (%[start_addr], %%r8), %%r9 \n"    /* r9: access loc */
                "xor %%r10, %%r10 \n"                                   /* r10: accessed size */
"LOOP_STRIDELOAD_MMAP_INNER: \n"                                             /* inner (access) loop, unroll 8 times */
                SIZEBTLD_4096_AVX512                                             /* Access: uses r10[size_accessed], r9 */
                "cmp %[accesssize], %%r10 \n"
                "jl LOOP_STRIDELOAD_MMAP_INNER \n"
		
		"xor %%r10, %%r10 \n"
"LOOP_STRIDELOAD_MMAP_DELAY: \n"                                             /* delay <delay> cycles */
                "inc %%r10 \n"
                "cmp %[delay], %%r10 \n"
                "jl LOOP_STRIDELOAD_MMAP_DELAY \n"

                "add %[skip], %%r8 \n"
                "inc %%r11 \n"
                "cmp %[count], %%r11 \n"

                "jl LOOP_STRIDELOAD_MMAP_OUTER \n"

                :: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), [skip]"r"(skip), [delay]"r"(delay):
                        "%r11", "%r10", "%r9", "%r8");

}






void stride_nt(char *start_addr, long size, long skip, long delay, long count)
{
	// ntstore "access_size" bytes starting from start_addr per 64B - uses vmovntdq
	// wait for "delay" cycles
	// then add "skip bytes" to start_addr
	// continue for "count" iterations

	asm volatile (
		"xor %%r8, %%r8 \n"						/* r8: access offset */
		"xor %%r11, %%r11 \n"					/* r11: counter */
		"movq %[start_addr], %%xmm0 \n"
// 1
"LOOP_STRIDENT_OUTER: \n"
		"lea (%[start_addr], %%r8), %%r9 \n"
		"xor %%r10, %%r10 \n"					/* r10: accessed size */
"LOOP_STRIDENT_INNER: \n"						/* inner (access) loop, unroll 8 times */
		SIZEBTNT_256_AVX512
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_STRIDENT_INNER \n"
		SIZEBTLD_FENCE

		"xor %%r10, %%r10 \n"
"LOOP_STRIDENT_DELAY: \n"
		"inc %%r10 \n"
		"cmp %[delay], %%r10 \n"
		"jl LOOP_STRIDENT_DELAY \n"

		"add %[skip], %%r8 \n"
		"inc %%r11 \n"
		"cmp %[count], %%r11 \n"

		"jl LOOP_STRIDENT_OUTER \n"

		:: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), [skip]"r"(skip), [delay]"r"(delay):
			"%r11", "%r10", "%r9", "%r8");
}

void stride_store(char *start_addr, long size, long skip, long delay, long count)
{

	// ntstore "access_size" bytes starting from start_addr per 64B - uses vmovdqa64
	// wait for "delay" cycles
	// then add "skip bytes" to start_addr
	// continue for "count" iterations

	//printf("start_addr is %p,  access_size is %ld, stride_size is %ld, delay is %ld, count is %ld\n",
	//			start_addr, size, skip, delay, count);

	asm volatile (
		"xor %%r8, %%r8 \n"						/* r8: access offset */
		"xor %%r11, %%r11 \n"					/* r11: counter */
		"movq %[start_addr], %%xmm0 \n"			/* zmm0: read/write register */
// 1
"LOOP_STRIDEST_OUTER: \n"						/* outer (counter) loop */
		"lea (%[start_addr], %%r8), %%r9 \n"	/* r9: access loc */
		"xor %%r10, %%r10 \n"					/* r10: accessed size */
"LOOP_STRIDEST_INNER: \n"						/* inner (access) loop, unroll 8 times */
		SIZEBTST_64_AVX512						/* Access: uses r10[size_accessed], r9 */
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_STRIDEST_INNER \n"
		SIZEBTST_FENCE

		"xor %%r10, %%r10 \n"
"LOOP_STRIDEST_DELAY: \n"						/* delay <delay> cycles */
		"inc %%r10 \n"
		"cmp %[delay], %%r10 \n"
		"jl LOOP_STRIDEST_DELAY \n"

		"add %[skip], %%r8 \n"
		"inc %%r11 \n"
		"cmp %[count], %%r11 \n"

		"jl LOOP_STRIDEST_OUTER \n"

		:: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), [skip]"r"(skip), [delay]"r"(delay):
			"%r11", "%r10", "%r9", "%r8");
	
}



void stride_ssd_store(char *start_addr, long size, long offset, long fd, long count) {
    size_t total = (size_t)(count * size);
    //char *buf = new char[total];
    ssize_t result = pwrite(fd, allocated_buf, total, offset);
    if (result == -1) {
        perror("pwrite");
        exit(1);
    }
    //delete [] buf;
}

void stride_ssd_store_write(char *start_addr, long size, long offset, long fd, long count) {
    size_t total = (size_t)(count * size);
    //char *buf = new char[total];
    write(fd, allocated_buf, total);
    //delete [] buf;
}



void stride_ssd_mmap_store(char *start_addr, long size, long offset, long fd, long count) {
   long skip = size;
   long delay = 0;

        asm volatile (
                "xor %%r8, %%r8 \n"                                             /* r8: access offset */
                "xor %%r11, %%r11 \n"                                   /* r11: counter */
                "movq %[start_addr], %%xmm0 \n"                 /* zmm0: read/write register */
// 1
"LOOP_STRIDEST_MMAP_OUTER: \n"                                               /* outer (counter) loop */
                "lea (%[start_addr], %%r8), %%r9 \n"    /* r9: access loc */
                "xor %%r10, %%r10 \n"                                   /* r10: accessed size */
"LOOP_STRIDEST_MMAP_INNER: \n"                                               /* inner (access) loop, unroll 8 times */
                SIZEBTST_4096_AVX512                                              /* Access: uses r10[size_accessed], r9 */
                "cmp %[accesssize], %%r10 \n"
                "jl LOOP_STRIDEST_MMAP_INNER \n"

                "xor %%r10, %%r10 \n"
"LOOP_STRIDEST_MMAP_DELAY: \n"                                               /* delay <delay> cycles */
                "inc %%r10 \n"
                "cmp %[delay], %%r10 \n"
                "jl LOOP_STRIDEST_MMAP_DELAY \n"

                "add %[skip], %%r8 \n"
                "inc %%r11 \n"
                "cmp %[count], %%r11 \n"

                "jl LOOP_STRIDEST_MMAP_OUTER \n"

                :: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), [skip]"r"(skip), [delay]"r"(delay):
                        "%r11", "%r10", "%r9", "%r8");

}


void stride_ssd_mmap_store_no_drain(char *start_addr, long size, long offset, long fd, long count) {
    size_t total = size * count;
    char* add = start_addr;
    //char *buff = new char[total];
    uint8_t* buf = allocated_buf;
    for (size_t i = 0 ; i < total ; ) {
        pmem_memcpy_nodrain(add, buf, size * sizeof(char));   
	add += size;
	i += size;
	buf +=size;
    }
    //delete [] buff;
}



void stride_storeclwb(char *start_addr, long size, long skip, long delay, long count)
{

	// ntstore "access_size" bytes starting from start_addr per 64B - uses vmovdqa64 + clwb per 64B
	// wait for "delay" cycles
	// then add "skip bytes" to start_addr
	// continue for "count" iterations

	asm volatile (
		"xor %%r8, %%r8 \n"						/* r8: access offset */
		"xor %%r11, %%r11 \n"					/* r11: counter */
		"movq %[start_addr], %%xmm0 \n"			/* zmm0: read/write register */
// 1
"LOOP_STRIDESTFLUSH_OUTER: \n"						/* outer (counter) loop */
		"lea (%[start_addr], %%r8), %%r9 \n"	/* r9: access loc */
		"xor %%r10, %%r10 \n"					/* r10: accessed size */
"LOOP_STRIDESTFLUSH_INNER: \n"						/* inner (access) loop, unroll 8 times */
		SIZEBTSTFLUSH_256_AVX512						/* Access: uses r10[size_accessed], r9 */
		"cmp %[accesssize], %%r10 \n"
		"jl LOOP_STRIDESTFLUSH_INNER \n"
		SIZEBTST_FENCE

		"xor %%r10, %%r10 \n"
"LOOP_STRIDESTFLUSH_DELAY: \n"						/* delay <delay> cycles */
		"inc %%r10 \n"
		"cmp %[delay], %%r10 \n"
		"jl LOOP_STRIDESTFLUSH_DELAY \n"

		"add %[skip], %%r8 \n"
		"inc %%r11 \n"
		"cmp %[count], %%r11 \n"

		"jl LOOP_STRIDESTFLUSH_OUTER \n"

		:: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), [skip]"r"(skip), [delay]"r"(delay):
			"%r11", "%r10", "%r9", "%r8");
}

#define RDRAND_MAX_RETRY 32

/* 
 * Generate random number to [rd] within [range], return 0 if success, 1 if fail.
 */
void stride_read_after_write(char *start_addr, long size, long skip, long delay, long count)
{
	asm volatile (
		"xor	%%r8, %%r8 \n"						/* r8: access offset */
		"xor	%%r11, %%r11 \n"					/* r11: counter */
		"movq	%[start_addr], %%xmm0 \n"			/* zmm0: read/write register */

"LOOP_RAW_OUTER: \n"								/* outer (counter) loop */
		"lea	(%[start_addr], %%r8), %%r9 \n"		/* r9: access loc */
		"xor	%%r10, %%r10 \n"					/* r10: accessed size */
"LOOP_RAW_STRIDESTCLWB_INNER: \n"					/* inner (access) loop, unroll 8 times */
		SIZEBTSTFLUSH_64_AVX512						/* Access: uses r10[size_accessed], r9 */
		"cmp	%[accesssize], %%r10 \n"
		"jl		LOOP_RAW_STRIDESTCLWB_INNER \n"
		"mfence \n"

		"xor	%%r10, %%r10 \n"
"LOOP_RAW_STRIDELDNT_INNER: \n"
		SIZEBTNT_64_AVX512
		"cmp	%[accesssize], %%r10 \n"
		"jl		LOOP_RAW_STRIDELDNT_INNER \n"
		"mfence \n"

		"xor	%%r10, %%r10 \n"
"LOOP_RAW_DELAY: \n"								/* delay <delay> cycles */
		"inc	%%r10 \n"
		"cmp	%[delay], %%r10 \n"
		"jl		LOOP_RAW_DELAY \n"

		"add	%[skip], %%r8 \n"
		"inc	%%r11 \n"
		"cmp	%[count], %%r11 \n"

		"jl		LOOP_RAW_OUTER \n"

		:: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), [skip]"r"(skip), [delay]"r"(delay):
			"%r11", "%r10", "%r9", "%r8");
}

static inline int get_rand(uint64_t *rd, uint64_t range)
{
    uint8_t ok;
    int i = 0;
    for (i = 0; i < RDRAND_MAX_RETRY; i++)
    {
        asm volatile("rdrand %0; setc %1\n\t"
                     : "=r"(*rd), "=qm"(ok));

        if (ok) {
            *rd = *rd % range;
			return 0;
        }
    }

	return 1;
}

int init_chasing_index(uint64_t *cindex, uint64_t csize)
{
	uint64_t curr_pos = 0;
	uint64_t next_pos = 0;
	uint64_t i = 0;
	int ret = 0;

	memset(cindex, 0, sizeof(uint64_t) * csize);

	for(i = 0; i < csize-1; i++)
	{
		do {
			ret = get_rand(&next_pos, csize);
			if(ret != 0)
				return 1;
		} while((cindex[next_pos] != 0) || (next_pos == curr_pos));

		cindex[curr_pos] = next_pos;
		curr_pos = next_pos;
	}

	return 0;
}

void chasing_storeclwb(char *start_addr, long size, long skip, long count, uint64_t *cindex)
{
	asm volatile (
		"xor	%%r8, %%r8 \n"						/* r8: access offset */
		"xor	%%r11, %%r11 \n"					/* r11: counter */
"LOOP_CHASING_STCLWB_OUTER: \n"						/* outer (counter) loop */
		"lea	(%[start_addr], %%r8), %%r9 \n"		/* r9: access loc */
		"xor	%%r10, %%r10 \n"					/* r10: accessed size */
		"xor	%%r12, %%r12 \n"    				/* r12: chasing index addr */
"LOOP_CHASING_STCLWB_INNER: \n"
		"movq	(%[cindex], %%r12, 8), %%xmm0\n"
		"shl    $0x06, %%r12\n"
		"vmovdqa64	%%zmm0,  0x0(%%r9, %%r12) \n"
		"clwb	0x0(%%r9, %%r12) \n"
		"add	$0x40, %%r10\n"
		"movq	%%xmm0, %%r12\n"					/* Update to next chasing element */

		"cmp	%[accesssize], %%r10 \n"
		"jl		LOOP_CHASING_STCLWB_INNER \n"
		SIZEBTST_FENCE

		"xor	%%r10, %%r10 \n"

		"add	%[skip], %%r8 \n"
		"inc	%%r11 \n"
		"cmp	%[count], %%r11 \n"

		"jl		LOOP_CHASING_STCLWB_OUTER \n"

		:
		: [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), [skip]"r"(skip), [cindex]"r"(cindex)
		: "%r12", "%r11", "%r10", "%r9", "%r8");
}

void chasing_loadnt(char *start_addr, long size, long skip, long count, uint64_t *cindex)
{
	asm volatile (
		"xor    %%r8, %%r8 \n"						/* r8: access offset */
		"xor    %%r11, %%r11 \n"					/* r11: counter */
"LOOP_CHASING_STRIDENT_OUTER: \n"				    /* outer (counter) loop */
		"lea    (%[start_addr], %%r8), %%r9 \n"	    /* r9: access loc */
		"xor    %%r10, %%r10 \n"					/* r10: accessed size */
		"xor	%%r12, %%r12 \n"    				/* r12: chasing index addr */
"LOOP_CHASING_STRIDENT_INNER: \n"
		"shl    $0x06, %%r12\n"
		"vmovntdqa 0x0(%%r9, %%r12), %%zmm0\n"
		"movq   %%xmm0, %%r12\n"                    /* Update to next chasing element */
		"add    $0x40, %%r10 \n"

		"cmp    %[accesssize], %%r10 \n"
		"jl     LOOP_CHASING_STRIDENT_INNER \n"
		SIZEBTLD_FENCE

		//"mfence \n"  /* !!!! */
		"add    %[skip], %%r8 \n"
		"inc    %%r11 \n"
		"cmp    %[count], %%r11 \n"

		"jl     LOOP_CHASING_STRIDENT_OUTER \n"

		:
        : [start_addr]"r"(start_addr), [accesssize]"r"(size), [count]"r"(count), [skip]"r"(skip), [cindex]"r"(cindex)
        : "%r11", "%r10", "%r9", "%r8");
}


void cachefence(char *start_addr, long size, long cache, long fence)
{
	asm volatile (
		"movq %[start_addr], %%xmm0 \n"
		"xor %%r9, %%r9 \n"					/* r9: offset of write */
"CACHEFENCE_FENCEBEGIN: \n"
		"xor %%r11, %%r11 \n"				/* r11: fence counter */
"CACHEFENCE_FLUSHBEGIN: \n"
		"xor %%r10, %%r10 \n"				/* r10: clwb counter */
//		"movq %%r9, %%rdx \n"				/* rdx: flush start offset */
		"leaq (%[start_addr], %%r9), %%rdx \n"
"CACHEFENCE_WRITEONE: \n"
		"vmovdqa64  %%zmm0, 0x0(%[start_addr], %%r9) \n"	/* Write one addr */
		"add $0x40, %%r9 \n"
		"add $0x40, %%r10 \n"
		"add $0x40, %%r11 \n"
		"cmp %[cache], %%r10 \n"		/* check clwb */
		"jl CACHEFENCE_WRITEONE \n"

		"leaq (%[start_addr], %%r9), %%rcx \n"					/* rcx: flush end offset, rdx->rcx */
//		"add %[start_addr], %%rcx"
"CACHEFENCE_FLUSHONE: \n"
		"clwb (%%rdx) \n"     /* Flush from rdx to rcx */
		"add $0x40, %%rdx \n"
		"cmp %%rcx, %%rdx \n"
		"jl CACHEFENCE_FLUSHONE \n"

		"cmp %[fence], %%r11 \n"
		"jl CACHEFENCE_FLUSHBEGIN \n"
		CACHEFENCE_FENCE

		"cmp %[accesssize], %%r9 \n"
		"jl CACHEFENCE_FENCEBEGIN \n"

		:: [start_addr]"r"(start_addr), [accesssize]"r"(size), [cache]"r"(cache), [fence]"r"(fence):
			"%rdx", "%rcx", "%r11", "%r10", "%r9");
	return;
}


void cacheprobe(char *start_addr, char *end_addr, long stride)
{
	asm volatile (
		"mov %[start_addr], %%r8 \n"
		"movq %[start_addr], %%xmm0 \n"
"LOOP_CACHEPROBE: \n"
		"vmovdqa64 %%zmm0, 0x0(%%r8) \n"
		"clflush (%%r8) \n"
		"vmovdqa64 %%zmm0, 0x40(%%r8) \n"
		"clflush 0x40(%%r8) \n"
		"add %[stride], %%r8 \n"
		"cmp %[end_addr], %%r8 \n"
		"jl LOOP_CACHEPROBE \n"
		"mfence \n"

		:: [start_addr]"r"(start_addr), [end_addr]"r"(end_addr), [stride]"r"(stride):
			"%r8");
	return;
}

void imcprobe(char *start_addr, char *end_addr, long loop)
{
	asm volatile (
		"xor %%r9, %%r9 \n"
		"movq %[start_addr], %%xmm0 \n"

"LOOP1_IMCPROBE: \n"
		"mov %[start_addr], %%r8 \n"
"LOOP2_IMCPROBE: \n"
		"vmovntdq %%zmm0, 0x0(%%r8) \n"
		"add $0x40, %%r8 \n"
		"cmp %[end_addr], %%r8 \n"
		"jl LOOP2_IMCPROBE \n"

		"add $1, %%r9 \n"
		"cmp %[loop], %%r9 \n"
		"jl LOOP1_IMCPROBE \n"

		:: [start_addr]"r"(start_addr), [end_addr]"r"(end_addr), [loop]"r"(loop):
			"%r8", "%r9");
	return;
}


void seq_load(char * start_addr, char *end_addr, long size) {

	// load with "vmovntdqa", until size bytes
	// repeat until reaching end_addr

	asm volatile (
		"mov %[start_addr], %%r9 \n"

"LOOP_SEQLOAD1: \n"
		"xor %%r8, %%r8 \n"
"LOOP_SEQLOAD2: \n"
		"vmovntdqa 0x0(%%r9, %%r8), %%zmm0 \n"
		"add $0x40, %%r8 \n"
		"cmp %[size], %%r8 \n"
		"jl LOOP_SEQLOAD2 \n"

		"add %[size], %%r9 \n"
		"cmp %[end_addr], %%r9 \n"
		"jl LOOP_SEQLOAD1 \n"

		:: [start_addr]"r"(start_addr), [end_addr]"r"(end_addr), [size]"r"(size):
			"%r8", "%r9");
	return;
}
void seq_store(char * start_addr, char *end_addr, long size) {

	// store with "vmovdqa64" + "clwb" per 64B, until size bytes
	// repeat until reaching end_addr

	//printf("start address is %p, end address is %p, size is %ld\n", start_addr, end_addr, size);

	asm volatile (
		"mov %[start_addr], %%r9 \n"
		"movq %[start_addr], %%xmm0 \n"
"LOOP_SEQSTORE1: \n"
		"xor %%r8, %%r8 \n"
"LOOP_SEQSTORE2: \n"
		"vmovdqa64  %%zmm0,  0x0(%%r9, %%r8) \n"
		"clwb  0x0(%%r9, %%r8) \n"
		"add $0x40, %%r8 \n"
		"cmp %[size], %%r8 \n"
		"jl LOOP_SEQSTORE2 \n"

		"add %[size], %%r9 \n"
		"cmp %[end_addr], %%r9 \n"
		"jl LOOP_SEQSTORE1 \n"

		:: [start_addr]"r"(start_addr), [end_addr]"r"(end_addr), [size]"r"(size):
			"%r8", "%r9");
	return;
}

void seq_clwb(char * start_addr, char *end_addr, long size) {

	// same as above?

	asm volatile (
		"mov %[start_addr], %%r9 \n"
		"movq %[start_addr], %%xmm0 \n"

"LOOP_SEQCLWB1: \n"
		"xor %%r8, %%r8 \n"
"LOOP_SEQCLWB2: \n"
		"vmovdqa64  %%zmm0,  0x0(%%r9, %%r8) \n"
		"clwb  0x0(%%r9, %%r8) \n"
		"add $0x40, %%r8 \n"
		"cmp %[size], %%r8 \n"
		"jl LOOP_SEQCLWB2 \n"

		"add %[size], %%r9 \n"
		"cmp %[end_addr], %%r9 \n"
		"jl LOOP_SEQCLWB1 \n"

		:: [start_addr]"r"(start_addr), [end_addr]"r"(end_addr), [size]"r"(size):
			"%r8", "%r9");
	
}

void seq_nt(char * start_addr, char *end_addr, long size) {

	// ntstore with "vmovntdq" per 64B, until size bytes
	// repeat until reaching end_addr

	asm volatile (
		"mov %[start_addr], %%r9 \n"
		"movq %[start_addr], %%xmm0 \n"

"LOOP_SEQNT1: \n"
		"xor %%r8, %%r8 \n"
"LOOP_SEQNT2: \n"
		"vmovntdq %%zmm0, 0x0(%%r9, %%r8) \n"
		"add $0x40, %%r8 \n"
		"cmp %[size], %%r8 \n"
		"jl LOOP_SEQNT2 \n"

		"add %[size], %%r9 \n"
		"cmp %[end_addr], %%r9 \n"
		"jl LOOP_SEQNT1 \n"

		:: [start_addr]"r"(start_addr), [end_addr]"r"(end_addr), [size]"r"(size):
			"%r8", "%r9");
}
