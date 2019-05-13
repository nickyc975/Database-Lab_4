#include <time.h>
#include <stdio.h>

#include "bptree.h"
#include "database.h"
#include "../lib/libextmem.h"

#define R_BLK_NUM 64
#define S_BLK_NUM 128
#define R_HEAD_BLK_ADDR 0x520000
#define S_HEAD_BLK_ADDR 0x530000
#define R_BPTREE_ROOT_ADDR 0x52000000
#define S_BPTREE_ROOT_ADDR 0x53000000

int gen_blocks(Buffer *buffer, database_t *R_db, database_t *S_db);

int main(int argc, char *argv[])
{
    Buffer _buffer;
    Buffer *buffer = &_buffer;
    initBuffer(520, 64, buffer);

    database_t R_db, S_db;
    R_db.type = R_T;
    R_db.buffer = buffer;
    R_db.head_blk_addr = R_HEAD_BLK_ADDR;
    R_db.bptree_meta.root_addr = R_BPTREE_ROOT_ADDR;
    R_db.bptree_meta.leftmost_addr = R_BPTREE_ROOT_ADDR;
    R_db.bptree_meta.last_alloc_addr = R_BPTREE_ROOT_ADDR;

    S_db.type = S_T;
    S_db.buffer = buffer;
    S_db.head_blk_addr = S_HEAD_BLK_ADDR;
    S_db.bptree_meta.root_addr = S_BPTREE_ROOT_ADDR;
    S_db.bptree_meta.leftmost_addr = S_BPTREE_ROOT_ADDR;
    S_db.bptree_meta.last_alloc_addr = S_BPTREE_ROOT_ADDR;

    srand((unsigned int)time(NULL));

    printf("numIO: %ld, numFreeBlk: %ld\n", buffer->numIO, buffer->numFreeBlk);

    gen_blocks(buffer, &R_db, &S_db);

    printf("numIO: %ld, numFreeBlk: %ld\n", buffer->numIO, buffer->numFreeBlk);

    linear_search(&R_db, 40, 0x5200);

    printf("numIO: %ld, numFreeBlk: %ld\n", buffer->numIO, buffer->numFreeBlk);

    linear_search(&S_db, 60, 0x5300);

    printf("numIO: %ld, numFreeBlk: %ld\n", buffer->numIO, buffer->numFreeBlk);

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

int gen_blocks(Buffer *buffer, database_t *R_db, database_t *S_db)
{
    addr_t blk_addr;
    bptree_t R_bptree, S_bptree;
    block_t *block = new_blk(buffer);

    bptree_init(&R_bptree, &(R_db->bptree_meta), buffer);

    for (int blk_num = 0; blk_num < R_BLK_NUM; blk_num++)
    {
        blk_addr = R_HEAD_BLK_ADDR + blk_num;
        while(block->tuple_num < TUPLES_PER_BLK)
        {
            gen_R(&(block->R_tuples[block->tuple_num]));
            bptree_insert(&R_bptree, block->R_tuples[block->tuple_num].A, blk_addr);
            block->tuple_num++;
        }

        if (blk_num < R_BLK_NUM - 1)
        {
            block->next_blk = blk_addr + 1;
        }
        else
        {
            block->next_blk = 0;
        }

        save_blk(buffer, block, blk_addr, 1);
        block = new_blk(buffer);
    }

    // bptree_print(&R_bptree);
    bptree_free(&R_bptree, &(R_db->bptree_meta));

    bptree_init(&S_bptree, &(S_db->bptree_meta), buffer);
    for (int blk_num = 0; blk_num < S_BLK_NUM; blk_num++)
    {
        blk_addr = S_HEAD_BLK_ADDR + blk_num;
        while(block->tuple_num < TUPLES_PER_BLK)
        {
            gen_S(&(block->S_tuples[block->tuple_num]));
            bptree_insert(&S_bptree, block->S_tuples[block->tuple_num].C, blk_addr);
            block->tuple_num++;
        }

        if (blk_num < S_BLK_NUM - 1)
        {
            block->next_blk = blk_addr + 1;
        }
        else
        {
            block->next_blk = 0;
        }

        save_blk(buffer, block, blk_addr, 1);
        block = new_blk(buffer);
    }

    // bptree_print(&S_bptree);
    bptree_free(&S_bptree, &(S_db->bptree_meta));
    free_blk(buffer, block);
    return 0;
}