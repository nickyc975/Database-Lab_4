#ifndef BPTREE_H
#define BPTREE_H

#include "../lib/libextmem.h"

#define MAX_VALUE_NUM 13

typedef struct node_struct node_t;

typedef struct key_iter_struct key_iter_t;

typedef struct value_blk_iter_struct value_blk_iter_t;

typedef struct value_blk_struct
{
    int value_num;
    addr_t blk_addr;
    addr_t next_blk_addr;
    addr_t values[MAX_VALUE_NUM];
} value_blk_t;

typedef struct bptree_meta_struct
{
    addr_t root_addr;
    addr_t leftmost_addr;
    addr_t last_alloc_addr;

    addr_t *leaf_addrs;
    unsigned int leaf_num;
} bptree_meta_t;

typedef struct bptree_struct
{
    node_t *root_node;
    node_t *leftmost_node;
    addr_t last_alloc_addr;
} bptree_t;

void bptree_init(bptree_t *bptree, bptree_meta_t *meta);

void bptree_insert(bptree_t *bptree, int key, addr_t value);

void bptree_delete(bptree_t *bptree, int key);

addr_t bptree_query(bptree_t *bptree, int key);

void bptree_print(bptree_t *bptree);

void bptree_getmeta(bptree_t *bptree, bptree_meta_t *meta);

void bptree_free(bptree_t *bptree);

key_iter_t *new_key_iter(bptree_meta_t *meta);

int has_next_key(key_iter_t *iter);

int next_key(key_iter_t *iter);

addr_t curr_blk_addr(key_iter_t *iter);

void free_key_iter(key_iter_t *iter);

node_t *read_node(addr_t addr);

addr_t node_get(node_t *node, int key);

int node_maxkey(node_t *node);

int node_minkey(node_t *node);

addr_t node_next_node(node_t *node);

void free_node(node_t *node);

value_blk_iter_t *new_value_blk_iter(addr_t addr);

void reset_value_blk_iter(value_blk_iter_t *iter);

int has_next_value(value_blk_iter_t *iter);

addr_t next_value(value_blk_iter_t *iter);

void free_value_blk_iter(value_blk_iter_t *iter);

value_blk_t *read_value_blk(addr_t blk_addr);

void free_value_blk(value_blk_t *value_blk);

#endif