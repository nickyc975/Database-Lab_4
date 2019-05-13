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

typedef struct block_struct
{
    int tuple_num;
    addr_t next_blk;

    union
    {
        S S_tuples[TUPLES_PER_BLK];
        R R_tuples[TUPLES_PER_BLK];
    };
} block_t;

typedef struct database_struct
{
    db_type type;
    Buffer *buffer;
    addr_t head_blk_addr;
    bptree_meta_t *bptree_meta;
} database_t;

block_t *new_blk(Buffer *buffer);

block_t *read_blk(Buffer *buffer, addr_t blk_addr);

int save_blk(Buffer *buffer, block_t *block, addr_t blk_addr, int free_after_save);

void free_blk(Buffer *buffer, block_t *block);

int linear_search(database_t *database, int key, addr_t base_addr);

int binary_search(database_t *database, int key, addr_t base_addr);

int index_search(database_t *database, int key, addr_t base_addr);

int project(database_t *database, addr_t base_addr);

int next_loop_join(database_t *R_db, database_t *S_db, addr_t base_addr);

int sort_merge_join(database_t *R_db, database_t *S_db, addr_t base_addr);

int hash_join(database_t *R_db, database_t *S_db, addr_t base_addr);

#endif