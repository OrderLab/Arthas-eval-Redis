// The Arthas Project
//
// Copyright (c) 2019, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef _REACTOR_CHECKPOINT_H_
#define _REACTOR_CHECKPOINT_H_

#include <libpmemobj.h>
#include <libpmem.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_VARIABLES 1000
#define MAX_VERSIONS 3
#define PMEM_LEN 200000

#define INT_CHECKPOINT 0
#define DOUBLE_CHECKPOINT 1
#define STRING_CHECKPOINT 2
#define BOOL_CHECKPOINT 3

//checkpoint log entry by logical sequence number
typedef struct single_data {
  const void *address;
  uint64_t offset;
  void *data;
  size_t size;
  int sequence_number;
  int version;
  int data_type;
  void *old_data[MAX_VERSIONS];
  size_t old_size[MAX_VERSIONS];
  uint64_t old_checkpoint_entry;
} single_data;

// checkpoint log entry by address/offset
typedef struct checkpoint_data {
  const void *address;
  uint64_t offset;
  void *data[MAX_VERSIONS];
  size_t size[MAX_VERSIONS];
  int version;
  int data_type;
  int sequence_number[MAX_VERSIONS];
  uint64_t old_checkpoint_entry;
} checkpoint_data;

typedef struct checkpoint_log {
  struct checkpoint_data c_data[MAX_VARIABLES];
  int variable_count;
} checkpoint_log;

struct checkpoint_log *reconstruct_checkpoint(const char *file_path,
                                              const char *pmem_library) {
  int variable_count;
  struct checkpoint_log *c_log;
  if (strcmp(pmem_library, "libpmemobj") == 0) {
    PMEMobjpool *pop = pmemobj_open(file_path, "checkpoint");
    if (!pop) {
      fprintf(stderr, "pool not found\n");
      pmemobj_errormsg();
      return NULL;
    }
    PMEMoid oid = pmemobj_root(pop, sizeof(uint64_t));
    uint64_t *old_pool = (uint64_t *)pmemobj_direct(oid);

    PMEMoid clog_oid = POBJ_FIRST_TYPE_NUM(pop, 0);
    c_log = (struct checkpoint_log *)pmemobj_direct(clog_oid);
    uint64_t offset;
    offset = (uint64_t)c_log->c_data - *old_pool;
    variable_count = c_log->variable_count;
    for (int i = 0; i < variable_count; i++) {
      // printf("variable %d\n", i);
      int data_index = c_log->c_data[i].version;
      // printf("total versions is %d\n", data_index);
      for (int j = 0; j <= data_index; j++) {
        // printf("version %d\n", j);
        offset = (uint64_t)c_log->c_data[i].data[j] - *old_pool;
        // printf("offset is %ld\n", offset);
        c_log->c_data[i].data[j] = (void *)((uint64_t)pop + offset);
      }
      printf("offset is %ld old checkpoint entry is %ld\n",
                 c_log->c_data[i].offset,
                 c_log->c_data[i].old_checkpoint_entry);
    }
    *old_pool = (uint64_t)pop;
  } else if (strcmp(pmem_library, "libpmem") == 0) {
    // TODO:open memory mapped file in a different manner
    char *pmemaddr;
    size_t mapped_len;
    int is_pmem;

    if ((pmemaddr = (char *)pmem_map_file(file_path, PMEM_LEN, PMEM_FILE_CREATE,
                                          0666, &mapped_len, &is_pmem)) ==
        NULL) {
      perror("pmem_mapping failure\n");
      exit(1);
    }
    c_log = (struct checkpoint_log *)pmemaddr;
    uint64_t *old_pool_ptr =
        (uint64_t *)((uint64_t)c_log + sizeof(struct checkpoint_log));
    uint64_t old_pool = *old_pool_ptr;

    uint64_t offset;
    variable_count = c_log->variable_count;
   // printf("variable_count %d\n", c_log->variable_count);
    // printf("old pool ptr is %ld\n", old_pool);
    // offset = (uint64_t)c_log->c_data[0].data[0] - old_pool;
    for (int i = 0; i < variable_count; i++) {
      // printf("version is %d\n", c_log->c_data[i].version);
      // printf("address is %p\n", c_log->c_data[i].address);
      for (int j = 0; j <= c_log->c_data[i].version; j++) {
        offset = (uint64_t)c_log->c_data[i].data[j] - old_pool;
        // printf("offset is %ld\n", offset);
        // printf("size is %ld\n", c_log->c_data[i].size[j]);
        c_log->c_data[i].data[j] = (void *)((uint64_t)c_log + offset);
        // printf("data is %s\n", (char *)c_log->c_data[i].data[j]);
      }
    }
  }
  if (c_log == NULL) {
    return NULL;
  }
  printf("RECONSTRUCTED CHECKPOINT COMPONENT:\n");
  printf("variable count is %d\n", variable_count);
  for (int i = 0; i < variable_count; i++) {
    printf("address is %p offset is %ld\n", c_log->c_data[i].address,
           c_log->c_data[i].offset);
    // printf("version is %d\n", c_log->c_data[i].version);
    int data_index = c_log->c_data[i].version;
    printf("old checkpoint entry is %ld ", c_log->c_data[i].old_checkpoint_entry);
    for (int j = 0; j <= data_index; j++) {
      printf("version is %d ", j);
      printf("size is %ld  ", c_log->c_data[i].size[0]);
      printf("seq num is %d  ", c_log->c_data[i].sequence_number[j]);
      if (c_log->c_data[i].size[0] == 4)
        printf("int value is %d\n", *((int *)c_log->c_data[i].data[j]));
      else if (c_log->c_data[i].size[0] == 8)
        printf("double value is %f\n", *((double *)c_log->c_data[i].data[j]));
      else if (c_log->c_data[i].size[0] == sizeof(unsigned short))
        printf("unsigned short value is %hu\n",
               *((unsigned short *)c_log->c_data[i].data[j]));
      else {
        printf("value is not int or double %s\n",
               (char *)c_log->c_data[i].data[j]);
      }
      // else
      //  printf("value is %s\n", (char *)c_log->c_data[i].data[j]);
      // printf("version is %d, value is %f or %d\n", j, *((double
      // *)c_log->c_data[i].data[j]),*((int *)c_log->c_data[i].data[j]));
    }
  }
  return c_log;
}

//struct checkpoint_log *reconstruct_checkpoint(const char *file_path, const char *pmem_library);
//void order_by_sequence_num(struct single_data * ordered_data, size_t *total_size, struct checkpoint_log *c_log);
//int sequence_comparator(const void *v1, const void * v2);

#ifdef __cplusplus
}
#endif

#endif /* _REACTOR_CHECKPOINT_H_ */
