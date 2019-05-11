#ifndef BPTREE_H
#define BPTREE_H

#include "../lib/libextmem.h"

typedef struct node_struct node_t;

typedef struct bptree_struct
{
    node_t *root;
    Buffer *buffer;
    addr_t curr_addr;
} bptree_t;

void bptree_init(bptree_t *bptree, Buffer *buffer);

void bptree_insert(bptree_t *bptree, int key, addr_t value);

void bptree_delete(bptree_t *bptree, int key);

void bptree_query(bptree_t *bptree, int key, addr_t base_addr);

void bptree_print(bptree_t *bptree);

void bptree_free(bptree_t *bptree);

#endif