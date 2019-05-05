#ifndef COMMON_H
#define COMMON_H

#include "inttypes.h"

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
    union
    {
        S S_data[7];
        R R_data[7];
    };

    uint64_t next_blk;
} Block;

void gen_R(R *r);

void gen_S(S *s);

int linear_search();

int binary_search();

int index_search();

#endif