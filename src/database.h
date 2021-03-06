#ifndef DATABASE_H
#define DATABASE_H

#include "bptree.h"
#include "../lib/libextmem.h"

#define TUPLES_PER_BLK 7

typedef enum
{
    R_T, S_T
} db_type;

typedef struct R_struct
{
    union
    {
        int A;
        int key;
    };

    int B; 
} R;

typedef struct S_struct
{
    union
    {
        int C;
        int key;
    };
    
    int D; 
} S;

typedef struct joined_tuple_struct
{
    R R_tuple;
    S S_tuple;
} joined_tuple_t;

typedef struct block_struct
{
    int tuple_num;
    addr_t next_blk;

    union
    {
        R R_tuples[TUPLES_PER_BLK];
        S S_tuples[TUPLES_PER_BLK];
        int tuples[TUPLES_PER_BLK * sizeof(R) / sizeof(int)];
        joined_tuple_t joined_tuples[TUPLES_PER_BLK / 2];
    };
} block_t;

typedef struct database_struct
{
    db_type type;
    addr_t head_blk_addr;
    unsigned int blk_num;
    unsigned int tuple_num;
    bptree_meta_t bptree_meta;
} database_t;

addr_t next_blk_addr(addr_t addr);

addr_t get_blk_addr(addr_t addr);

addr_t get_blk_offset(addr_t addr);

block_t *new_blk();

block_t *read_blk(addr_t blk_addr);

int save_blk(block_t *block, addr_t blk_addr, int free_after_save);

void free_blk(block_t *block);

int linear_search(database_t *database, int key, addr_t base_addr);

int binary_search(database_t *database, int key, addr_t base_addr);

int index_search(database_t *database, int key, addr_t base_addr);

int project(database_t *database, addr_t base_addr);

int nest_loop_join(database_t *R_db, database_t *S_db, addr_t base_addr);

int sort_merge_join(database_t *R_db, database_t *S_db, addr_t base_addr);

int hash_join(database_t *R_db, database_t *S_db, addr_t base_addr);

#endif