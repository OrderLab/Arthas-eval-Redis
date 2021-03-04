
#ifndef __BOOK_H
#define __BOOK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct bookKeeper {
    uint64_t sds_offset;
    uint64_t sds_robj_offset;
    uint64_t val_offset;
    uint64_t val_robj_offset;
    uint64_t dict_offset;
};

#endif