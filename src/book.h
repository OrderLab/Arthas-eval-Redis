
#ifndef __BOOK_H
#define __BOOK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <libpmemobj.h>
struct bookKeeper {
    uint64_t sds_offset;
    uint64_t sds_robj_offset;
    uint64_t val_offset;
    uint64_t val_robj_offset;
    uint64_t dict_offset;
};

struct dictServer {
	char * pm_file_path;
        int pm_file_size;
        PMEMobjpool *pm_pool;
        uint64_t pool_uuid;
};

extern struct dictServer dstemp;

#endif
