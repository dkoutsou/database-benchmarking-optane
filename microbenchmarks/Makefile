
#obj-m += repfs.o latfs.o
CFLAGS=-pthread -latomic  -lpmem -lnuma #-march=native -mtune=native -ffast-math -DAEP_SUPPORTED
ccflags-y+=-mmmx -msse2 -mavx -mpreferred-stack-boundary=4
#repfs-y := rep.o
#latfs-y := lat.o proc.o misc.o memaccess.o tasks.o

avx_supported := $(call as-instr,vpxor %xmm0$(comma)%xmm0$(comma)%xmm0,yes,no)
avx2_supported := $(call as-instr,vpgatherdd %ymm0$(comma)(%eax$(comma)%ymm1\
	$(comma)4)$(comma)%ymm2,yes,no)

AEP := $(shell cat /proc/cpuinfo  | grep clwb | wc -l)

ifeq ($(AEP), 0)
  ccflags-y+= -DNOAVX512
else
  ccflags-y+= -DAEP_SUPPORTED
endif

# don't use perf for now
#ifeq ($(PERF), 1)
#  ccflags-y+=-DUSE_PERF
#  latfs-y+=perf_util.o
#endif

# what is this doing?
ccflags-y+= -DFENCE

# The following are from the original code -why compile with O2?
# all:
#	gcc -O2 -o gen_tasks gen_tasks.c $(ccflags-y)  
#
# debug:
#	gcc -O0 -o debug -mavx -g debug.c memaccess.c  $(ccflags-y)
#
#clean:
#	make -C /lib/modules/$(shell uname -r)/build M=`pwd` clean
#	rm -f tasks.py gen_tasks debug *.pyc -f 

#####################################################################

# ADDED #

ccflags-y+= -pthread -latomic -lpmem # -march=native -mtune=native -ffast-math

memaccess.o: memaccess.cc 
	g++ -O3 $(CFLAGS) -c memaccess.cc -o memaccess.o	

run_thread.o: run_thread.cc
	g++ -O3 $(CFLAGS) -c run_thread.cc -o run_thread.o	

benchmark.o: benchmark.cc 
	g++ -O3 $(CFLAGS) -c benchmark.cc -o benchmark.o

benchmark: run_thread.o memaccess.o benchmark.o 
	g++ -O3 run_thread.o memaccess.o benchmark.o $(CFLAGS) -o benchmark

all: benchmark

clean:
	rm memaccess.o run_thread.o benchmark.o benchmark
