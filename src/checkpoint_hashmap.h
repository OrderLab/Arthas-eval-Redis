#ifndef CHECKPOINT_HASHMAP_H
#define CHECKPOINT_HASHMAP_H 1

#define MAX_VARIABLES 5000010
#define LIST_VARIABLES 10000
#define MAX_VERSIONS 3
#define PMEM_LEN 8388608000
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include "libpmemobj.h"
#include <limits.h>
#include "libpmem.h"
//#include "pmem.h"

#define INT_CHECKPOINT 0
#define DOUBLE_CHECKPOINT 1
#define STRING_CHECKPOINT 2
#define BOOL_CHECKPOINT 3

struct pool_info {
  PMEMobjpool *pm_pool;
};

struct single_data {
  const void *address;
  uint64_t offset;
  void *data;
  size_t size;
  int sequence_number;
  int version;
  int data_type;
};

struct checkpoint_data {
  const void *address;
  uint64_t offset;
  void *data[MAX_VERSIONS];
  size_t size[MAX_VERSIONS];
  int version;
  int data_type;
  int sequence_number[MAX_VERSIONS];
  uint64_t old_checkpoint_entry;
  uint64_t new_checkpoint_entry;
  int free_flag;
  int tx_id[MAX_VERSIONS];
  //uint64_t old_checkpoint_entries[MAX_VERSIONS];
  //int old_checkpoint_counter;
};

/*struct checkpoint_log{
  struct checkpoint_data c_data[MAX_VARIABLES];
  int variable_count;
};*/

struct node{
    uint64_t offset;
    struct checkpoint_data c_data;
    struct node *next;
};

struct checkpoint_log {
  size_t size;
  int variable_count;
  struct node ** list;
};


extern int variable_count;
extern void *pmem_file_ptr;
extern struct pool_info settings ;
extern int non_checkpoint_flag;

//void write_flag(char c);
//int check_address_length(const void *address, size_t size);
//int search_for_offset(uint64_t pool_base, uint64_t offset);
//int search_for_address(const void *address);
void print_checkpoint_log(struct checkpoint_log *c_log);
//void revert_by_address(const void *address, int variable_index, int version, int type, size_t size);
//int check_offset(uint64_t offset, size_t size);
//void revert_by_offset(const void *address, uint64_t offset, int variable_index, int version, int type, size_t size);
//void order_by_sequence_num(struct single_data * ordered_data, size_t *total_size);
//int sequence_comparator(const void *v1, const void * v2);
//void print_sequence_array(struct single_data *ordered_data, size_t total_size);
//void checkpoint_realloc(void *new_ptr, void *old_ptr, uint64_t new_offset, uint64_t old_offset);
//void checkpoint_free(uint64_t off);

struct checkpoint_log *reconstruct_checkpoint(const char *file_path,
                                              const char *pmem_library) {
  printf("reconstruct\n");
  int variable_count;
  struct checkpoint_log *c_log;
/*  if (strcmp(pmem_library, "libpmemobj") == 0) {
    PMEMobjpool *pop = pmemobj_open(file_path, "checkpoint");
    if (!pop) {
      fprintf(stderr, "pool not found\n");
      pmemobj_errormsg();
      return NULL;
    }


    PMEMoid oid = pmemobj_root(pop, sizeof(uint64_t));
    uint64_t *old_pool = (uint64_t *)pmemobj_direct(oid);
    printf("old_pool is %p\n", (void *)*old_pool);
    PMEMoid clog_oid = POBJ_FIRST_TYPE_NUM(pop, 0);
    c_log = (struct checkpoint_log *)pmemobj_direct(clog_oid);
    uint64_t offset;
    printf("done with c_log\n");
    offset = (uint64_t)c_log->list - (uint64_t)pop;
    offset = (uint64_t)c_log->list - *old_pool;
    variable_count = c_log->variable_count;
    printf("c log list offset is %ld\n", offset);
    c_log->list = (struct node *) ((uint64_t)pop + offset);
    printf("fixed list\n");
   printf("variable count is %d\n", variable_count);


    struct node *list;
    struct node *temp;
    struct node * next_node;
    for (int i = 0; i < (int)c_log->size; i++) {
     list = c_log->list[i];
      temp = list;
      //printf("list is %p\n", c_log->list);
      //printf("list is done\n");
      //offset = (uint64_t)temp - *old_pool;
      //temp = (struct node *)((uint64_t)pop + offset);
      if(temp){
        offset = (uint64_t)temp - *old_pool;
        temp = (struct node *)((uint64_t)pop + offset);
        c_log->list[i] = (struct node *)((uint64_t)pop + offset);
      }
      while(temp){
        //printf("temp %d\n", i);
        int data_index = temp->c_data.version;
        //printf("address is %p offset is %ld\n", temp->c_data.address, temp->o$
        //printf("data index is %d\n", data_index);
        //printf("temp is %p\n", temp);
        for(int j = 0; j <= data_index; j++){
          offset = ((uint64_t)temp->c_data.data[j] - *old_pool);
          temp->c_data.data[j] = (void *)((uint64_t)pop + offset);
        }
        if(temp->next){
          offset = (uint64_t)temp->next - *old_pool;
          temp->next = (struct node *)((uint64_t)pop + offset);
        }
     temp = temp->next;
      }
    }
     *old_pool = (uint64_t)pop;
  }
  if (c_log == NULL) {
    return NULL;
  }*/
   printf("enter libpmem\n");
    char *pmemaddr;
    size_t mapped_len;
    int is_pmem;
    if ((pmemaddr = (char *)pmem_map_file(file_path, PMEM_LEN, PMEM_FILE_CREATE,
                                          0666, &mapped_len, &is_pmem)) ==
        NULL) {
      perror("pmem_mapping failure\n");
      exit(1);
    }
    printf("before addr\n");
    c_log = (struct checkpoint_log *)pmemaddr;
    uint64_t *old_pool_ptr =
        (uint64_t *)((uint64_t)c_log + sizeof(struct checkpoint_log));
    uint64_t old_pool = *old_pool_ptr;
    printf("old pool and c_log %ld %p\n", old_pool, (void *)old_pool);
    uint64_t offset;
    variable_count = c_log->variable_count;
    printf("variable count is %d\n", variable_count);
    offset = (uint64_t)c_log->list - old_pool;
    c_log->list = (struct node *)((uint64_t)pmemaddr + offset);
    printf("c_log->list is %p %ld\n", c_log->list, offset);
    struct node *list;
    struct node *temp;
    struct node *next_node;
    printf("after list\n");
    for (int i = 0; i < (int)c_log->size; i++) {
      // printf("i is %d\n", i);
      list = c_log->list[i];
      temp = list;
      // printf("temp is %p\n", temp);
      // printf("list is %p\n", c_log->list);
      // printf("list is done\n");
      // offset = (uint64_t)temp - *old_pool;
      // temp = (struct node *)((uint64_t)pop + offset);
      if (temp) {
       // printf("inside first\n");
        offset = (uint64_t)temp - old_pool;
        temp = (struct node *)((uint64_t)pmemaddr + offset);
        c_log->list[i] = (struct node *)((uint64_t)pmemaddr + offset);
      }
      while (temp) {
        // printf("inside temp\n");
        int data_index = temp->c_data.version;
        // printf("address is %p offset is %ld\n", temp->c_data.address,
        // temp->offset);
        // printf("data index is %d\n", data_index);
        // printf("temp is %p\n", temp);
        for (int j = 0; j <= data_index; j++) {
          offset = ((uint64_t)temp->c_data.data[j] - old_pool);
          temp->c_data.data[j] = (void *)((uint64_t)pmemaddr + offset);
          // printf("version is %d size is %ld value is %f or %d\n", j,
          // temp->c_data.size[j], *((double *)temp->c_data.data[j]),
          // *((int *)temp->c_data.data[j]));
          // printf("transaction id is %d\n", temp->c_data.tx_id[j]);
        }
        if (temp->next) {
          offset = (uint64_t)temp->next - old_pool;
          temp->next = (struct node *)((uint64_t)pmemaddr + offset);
        }
        temp = temp->next;
      }
    }
    // printf("finish\n");
    *old_pool_ptr = (uint64_t)pmemaddr;
  if (c_log == NULL) {
    return NULL;
  }
  printf("RECONSTRUCTED CHECKPOINT COMPONENT:\n");
 // printf("variable count is %d\n", variable_count);
  //print_checkpoint_log(c_log);
  printf("finish print\n");
  return c_log;
}

struct node * listlookup (uint64_t offset, struct node **n_list){
  int pos = listhashCode(offset);
  struct node *list = n_list[pos];
  struct node *temp = list;
  while (temp){
    if(temp->offset == offset){
      return temp;
    }
    temp = temp->next;
  }
  return NULL;
}

int hashCode (uint64_t offset, struct checkpoint_log *c_log){
  int ret_val;
  if (offset < 0){
    ret_val = (int)(offset%c_log->size);
    ret_val = -ret_val;
  }
  ret_val = (int)(offset%c_log->size);
  return ret_val;
}

struct node * lookup (uint64_t offset, struct checkpoint_log *c_log){
  int pos = hashCode(offset, c_log);
  struct node *list = c_log->list[pos];
  struct node *temp = list;
  while (temp){
    if(temp->offset == offset){
      return temp;
    }
    temp = temp->next;
  }
  return NULL;
}

void memory_leak_handle(void **n_list,
                        int num_recovered_addresses,
                        struct checkpoint_log *c_log,
                        PMEMobjpool *pop, uint64_t prev_pool){
  struct node *list;
  struct node *temp;
  int found = 0;
  void * new_address;
  struct node * n_search;
  for(int i = 0; i < (int)c_log->size; i++){
    list = c_log->list[i];
    temp = list;
    while(temp){
      found = 0;
      //printf("temp->offset is %ld\n", temp->offset);
      n_search = listlookup(temp->offset, n_list);
      if(!n_search){
        printf("memory leak found %p\n", temp->c_data.address);
        new_address = (uint64_t)temp->c_data.address - (prev_pool) + (uint64_t)pop;
        PMEMoid oid = pmemobj_oid(new_address);
	printf("type is %ld offset is %ld\n", pmemobj_type_num(oid), temp->offset);
        pmemobj_free(&oid);
      }
      /*for(int j = 0; j < num_recovered_addresses; j++){
        //printf("recovered address is %p\n", reconstructed_addresses[j]);
        if(temp->c_data.address == reconstructed_addresses[j]){
          found = 1;
          break;
        }
      }*/

      /*if(!found){
        printf("memory leak found %p\n", temp->c_data.address);
        new_address = (uint64_t)temp->c_data.address - (prev_pool) + (uint64_t)pop;
        PMEMoid oid = pmemobj_oid(new_address);
        pmemobj_free(&oid);
      }*/
      temp = temp->next;
    }
  }

}

void print_checkpoint_log(struct checkpoint_log *c_log){
  //if(sequence_number <= 4131080 && sequence_number != 3999993)
  //  return;
  printf("**************\n\n");
  struct node *list;
  struct node *temp;
  for(int i = 0; i < (int)c_log->size; i++){
    list = c_log->list[i];
    temp = list;
    while(temp){
      //if(temp->c_data.offset == 223388685424 || temp->c_data.offset == 2234515207$
      printf("position is %d\n", i);
      printf("address is %p offset is %ld\n", temp->c_data.address, temp->offset);
      int data_index = temp->c_data.version;
      printf("number of versions is %d\n", temp->c_data.version);
      for(int j = 0; j <= data_index; j++){
        printf("version is %d size is %ld value is %f or %d\n", j,
        temp->c_data.size[j], *((double *)temp->c_data.data[j]),
         *((int *)temp->c_data.data[j]));
         int *ref = (int*)((uint64_t)temp->c_data.data[j] + 4 );
        printf("refcount could be %d\n", *ref);
      }
      //}
      temp = temp->next;
    }
  }
  //printf("the variable count is %d %d\n", variable_count, c_log->variable_count);
}

int listhashCode (uint64_t offset){
  int ret_val;
  if (offset < 0){
    ret_val = (int)(offset%LIST_VARIABLES);
    ret_val = -ret_val;
  }
  ret_val = (int)(offset%LIST_VARIABLES);
  return ret_val;
}

void insert ( uint64_t offset, struct checkpoint_data c_data, 
             struct node **n_list) {
  int pos = listhashCode (offset);
  struct node *list = n_list[pos];
  struct node *temp = list;
  uint64_t old_offset = 0;
  while (temp){
    if (temp->offset == offset){
      return;
    }
    temp = temp->next;
  }
  // Need to create a new insertion
  struct node *newNode = malloc(sizeof(struct node *));
  //newNode->c_data = c_data;
  newNode->next = list;
  newNode->offset = offset;
  n_list[pos] = newNode;

}

/*void comparison_checkpoint(struct checkpoint_log *c_log,
                           void ** recovered_addresses,
                           int num_rec, PMEMobjpool *pop){
  if(c_log == NULL)
    return;
  TX_BEGIN(pop){
  int variable_count = c_log->variable_count;
  int *recovered = malloc(sizeof(int)*variable_count);
  struct node ** biglist = malloc(sizeof(struct node *) * LIST_VARIABLES);
  for (int i = 0; i < LIST_VARIABLES; i++){
    list[i] = NULL;
  }
  for(int i = 0; i < num_rec; i++){
    for(int j = 0; j < variable_count; j++){
      if(c_log->c_data[j].offset == ((uint64_t)recovered_addresses[i]- (uint64_t)pop) ){
        recovered[j] = 1;
      }
    }
  }
  struct node *list;
  struct node *temp;
  for(int i = 0; i < (int)c_log->size; i++){
    list = c_log->list[i];
    temp = list;
    while(temp){
      //printf("address is %p offset is %ld\n", temp->c_data.address, temp->offset);
      int data_index = temp->c_data.version;
      //printf("number of versions is %d\n", temp->c_data.version);
      //for(int j = 0; j <= data_index; j++){
      //  printf("version is %d size is %ld value is %f or %d\n", j,
      //  temp->c_data.size[j], *((double *)temp->c_data.data[j]),
      //   *((int *)temp->c_data.data[j]));
      //   int *ref = (int*)((uint64_t)temp->c_data.data[j] + 4 );
      //  printf("refcount could be %d\n", *ref);
     // }
	struct node * search = listlookup(temp->offset, biglist);
      if(temp->c_data.offset == ((uint64_t)recovered_addresses[i]- (uint64_t)pop) ){
	//recovered[i] = 1;
	//insert(temp->c_data.offset, c_data, biglist);
	if(temp->c_data.free_flag != 1
      }
    }
  }

  for(int i = 0; i < variable_count; i++){
    if(recovered[i] != 1 && c_log->c_data[i].free_flag != 1){
      printf("memory leak, we are freeing %d %p\n", i,
               c_log->c_data[i].address);
      PMEMoid oid = pmemobj_oid(c_log->c_data[i].address);
      pmemobj_tx_free(oid);
    }
  }
  }TX_ONABORT{
    printf("comparison abortion\n");
  }TX_END
}*/

#endif
