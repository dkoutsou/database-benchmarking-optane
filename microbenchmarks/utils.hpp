#include <stdint.h>

#define PRETHREAD_BIT			31 /* 2GB/Thread */
#define PERTHREAD_WORKSET		(1L << PRETHREAD_BIT)
#define PERTHREAD_CHECKSTOP		(1L << 25) /* yield after wrtting 32MB */
                                            // ????????????????????
#define LATENCY_OPS_COUNT		1048576L
#define BASIC_OP_MASK			0x3FFFFFC0

struct worker_ctx {
	int core_id;
	int tid;
	char *addr;
	char* loc;
	uint8_t *seed_addr; /* for multi-threaded tasks only */
	long access_size;
        int op;
        int fd;
};
