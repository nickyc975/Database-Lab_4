#include <time.h>
#include <stdio.h>

#include "bptree.h"
#include "database.h"
#include "../lib/libextmem.h"

#define R_BLK_NUM 16
#define S_BLK_NUM 32
#define R_ADDR_PREFIX 0x520000
#define S_ADDR_PREFIX 0x530000
#define R_BPTREE_ROOT_ADDR 0x52000000
#define S_BPTREE_ROOT_ADDR 0x53000000

int gen_blocks(Buffer *buffer);

int main(int argc, char *argv[])
{
    Buffer _buffer;
    Buffer *buffer = &_buffer;
    initBuffer(520, 64, buffer);

    srand((unsigned int)time(NULL));

    printf("io num: %ld\n", buffer->numIO);
    gen_blocks(buffer);
    printf("io num: %ld\n", buffer->numIO);

    return 0;
}

void gen_R(R *r)
{
    r->A = rand() % 40 + 1;
    r->B = rand() % 1000 + 1;
}

void gen_S(S *s)
{
    s->C = rand() % 40 + 21;
    s->D = rand() % 1000 + 1;
}

int gen_blocks(Buffer *buffer)
{
    addr_t blk_addr;
    bptree_t R_bptree, S_bptree;
    bptree_meta_t R_meta, S_meta;
    block_t *block = new_blk(buffer);

    R_meta.root_addr = R_BPTREE_ROOT_ADDR;
    R_meta.leftmost_addr = R_BPTREE_ROOT_ADDR;
    R_meta.last_alloc_addr = R_BPTREE_ROOT_ADDR;
    bptree_init(&R_bptree, &R_meta, buffer);

    for (int blk_num = 0; blk_num < R_BLK_NUM; blk_num++)
    {
        blk_addr = R_ADDR_PREFIX + blk_num;
        for (int i = 0; i < TUPLES_PER_BLK; i++)
        {
            gen_R(&(block->R_tuples[i]));
            bptree_insert(&R_bptree, block->R_tuples[i].A, blk_addr);
        }

        if (blk_num < R_BLK_NUM - 1)
        {
            block->next_blk = blk_num + 1;
        }
        else
        {
            block->next_blk = 0;
        }

        save_blk(buffer, block, blk_addr, 1);
        block = new_blk(buffer);
    }

    bptree_print(&R_bptree);
    bptree_free(&R_bptree, &R_meta);

    S_meta.root_addr = S_BPTREE_ROOT_ADDR;
    S_meta.leftmost_addr = S_BPTREE_ROOT_ADDR;
    S_meta.last_alloc_addr = S_BPTREE_ROOT_ADDR;
    bptree_init(&S_bptree, &S_meta, buffer);
    for (int blk_num = 0; blk_num < S_BLK_NUM; blk_num++)
    {
        for (int i = 0; i < TUPLES_PER_BLK; i++)
        {
            gen_S(&(block->S_tuples[i]));
            bptree_insert(&S_bptree, block->S_tuples[i].C, blk_addr);
        }

        if (blk_num < S_BLK_NUM - 1)
        {
            block->next_blk = blk_num + 1;
        }
        else
        {
            block->next_blk = 0;
        }

        save_blk(buffer, block, blk_addr, 1);
        block = new_blk(buffer);
    }

    bptree_print(&S_bptree);
    bptree_free(&S_bptree, &S_meta);
    free_blk(buffer, block);

    printf("numFreeBlk: %ld\n", buffer->numFreeBlk);

    return 0;
}