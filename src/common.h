#ifndef COMMON_H
#define COMMON_H

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
    union
    {
        S S_data[TUPLES_PER_BLK];
        R R_data[TUPLES_PER_BLK];
    };

    unsigned int unused;
    unsigned int next_blk;
} Block;

void gen_R(R *r);

void gen_S(S *s);

int linear_search();

int binary_search();

int index_search();

#endif