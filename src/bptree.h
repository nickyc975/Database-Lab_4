#ifndef BPTREE_H
#define BPTREE_H

#include "common.h"

typedef enum
{
    INNER, LEAF, OVERFLOW
} node_type;

typedef struct value_struct
{
    union
    {
        R R_value;
        S S_value;
    };
} value_t;

typedef struct inner_node_struct 
{
    int keys[7];
    unsigned int addrs[8];
    unsigned int next_node;
} inner_node_t;

typedef struct leaf_node_struct
{
    value_t values[7];
    unsigned int next_node;
} leaf_node_t;

typedef struct node_struct
{
    int key_num;
    node_type type;
    union
    {
        inner_node_t *inner_node;
        leaf_node_t *leaf_node;
    };
} node_t;

typedef struct bptree_struct bptree_t;

void bptree_init(bptree_t *bptree);

void bptree_insert(int key, value_t *value);

void bptree_delete(int key);

void bptree_query(int key, unsigned int base_addr);

void bptree_free(bptree_t *bptree);

#endif