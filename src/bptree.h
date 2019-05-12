#ifndef BPTREE_H
#define BPTREE_H

#include "../lib/libextmem.h"

#define MAX_VALUE_NUM 13

typedef struct node_struct node_t;

typedef struct data_blk_struct
{
    int value_num;
    addr_t blk_addr;
    addr_t next_blk_addr;
    addr_t values[MAX_VALUE_NUM];
} data_blk_t;

typedef struct bptree_meta_struct
{
    addr_t root_addr;
    addr_t leftmost_addr;
    addr_t last_alloc_addr;
} bptree_meta_t;

typedef struct bptree_struct
{    
    Buffer *buffer;
    node_t *root_node;
    node_t *leftmost_node;
    addr_t last_alloc_addr;
} bptree_t;

void bptree_init(bptree_t *bptree, bptree_meta_t *meta, Buffer *buffer);

void bptree_insert(bptree_t *bptree, int key, addr_t value);

void bptree_delete(bptree_t *bptree, int key);

addr_t bptree_query(bptree_t *bptree, int key);

void bptree_print(bptree_t *bptree);

void bptree_free(bptree_t *bptree, bptree_meta_t *meta);

data_blk_t *read_data_blk(bptree_t *bptree, addr_t blk_addr);

void free_data_blk(bptree_t *bptree, data_blk_t *data_blk);

#endif