#ifndef BPTREE_H
#define BPTREE_H

#include "common.h"
#include "../lib/libextmem.h"

typedef struct bptree_struct bptree_t;

void bptree_init(bptree_t *bptree, Buffer *buffer);

void bptree_insert(bptree_t *bptree, int key, int value);

void bptree_delete(bptree_t *bptree, int key);

void bptree_query(bptree_t *bptree, int key, addr_t base_addr);

void bptree_free(bptree_t *bptree);

#endif