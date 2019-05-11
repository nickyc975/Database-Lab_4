#ifndef DATABASE_H
#define DATABASE_H

#include "../lib/libextmem.h"

#define TUPLES_PER_BLK 7

typedef struct R_struct
{
    int A;
    int B; 
} R;

typedef struct S_struct
{
    int C;
    int D; 
} S;

typedef struct Block_struct
{
    int tuple_num;
    addr_t next_blk;

    union
    {
        S S_tuples[TUPLES_PER_BLK];
        R R_tuples[TUPLES_PER_BLK];
    };
} Block;

int linear_search(int key);

int binary_search(int key);

int index_search(int key);

int project();

int next_loop_join();

int sort_merge_join();

int hash_join();

#endif