#include <time.h>
#include <stdio.h>

#include "bptree.h"
#include "database.h"
#include "../lib/libextmem.h"

#define R_BLK_NUM 16
#define S_BLK_NUM 32
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
    R_db.blk_num = R_BLK_NUM;
    R_db.tuple_num = R_BLK_NUM * TUPLES_PER_BLK;
    R_db.head_blk_addr = R_HEAD_BLK_ADDR;
    R_db.bptree_meta.root_addr = R_BPTREE_ROOT_ADDR;
    R_db.bptree_meta.leftmost_addr = R_BPTREE_ROOT_ADDR;
    R_db.bptree_meta.last_alloc_addr = R_BPTREE_ROOT_ADDR;

    S_db.type = S_T;
    S_db.buffer = buffer;
    S_db.blk_num = S_BLK_NUM;
    S_db.tuple_num = S_BLK_NUM * TUPLES_PER_BLK;
    S_db.head_blk_addr = S_HEAD_BLK_ADDR;
    S_db.bptree_meta.root_addr = S_BPTREE_ROOT_ADDR;
    S_db.bptree_meta.leftmost_addr = S_BPTREE_ROOT_ADDR;
    S_db.bptree_meta.last_alloc_addr = S_BPTREE_ROOT_ADDR;

    int count;
    unsigned long io_num;
    srand((unsigned int)time(NULL));
    gen_blocks(buffer, &R_db, &S_db);

    io_num = buffer->numIO;
    count = linear_search(&R_db, 40, 0x5200);
    printf("linear search in R found %d tuples, io cost: %ld\n", count, buffer->numIO - io_num);

    io_num = buffer->numIO;
    count = linear_search(&S_db, 60, 0x5300);
    printf("linear search in S found %d tuples, io cost: %ld\n", count, buffer->numIO - io_num);

    io_num = buffer->numIO;
    count = binary_search(&R_db, 40, 0x6200);
    printf("binary search in R found %d tuples, io cost: %ld\n", count, buffer->numIO - io_num);

    io_num = buffer->numIO;
    count = binary_search(&S_db, 60, 0x6300);
    printf("binary search in S found %d tuples, io cost: %ld\n", count, buffer->numIO - io_num);

    io_num = buffer->numIO;
    count = index_search(&R_db, 40, 0x7200);
    printf("index search in R found %d tuples, io cost: %ld\n", count, buffer->numIO - io_num);

    io_num = buffer->numIO;
    count = index_search(&S_db, 60, 0x7300);
    printf("index search in S found %d tuples, io cost: %ld\n", count, buffer->numIO - io_num);

    io_num = buffer->numIO;
    count = project(&R_db, 0x8200);
    printf("project from R to A created %d tuples, io cost: %ld\n", count, buffer->numIO - io_num);

    io_num = buffer->numIO;
    count = project(&S_db, 0x8300);
    printf("project from S to C created %d tuples, io cost: %ld\n", count, buffer->numIO - io_num);

    io_num = buffer->numIO;
    count = nest_loop_join(&R_db, &S_db, 0x9500);
    printf("nest loop join R and S created %d tuples, io cost: %ld\n", count, buffer->numIO - io_num);

    io_num = buffer->numIO;
    count = sort_merge_join(&R_db, &S_db, 0x9500);
    printf("sort merge join R and S created %d tuples, io cost: %ld\n", count, buffer->numIO - io_num);

    printf("numFreeBlk: %ld\n", buffer->numFreeBlk);

    if (R_db.bptree_meta.leaf_addrs)
    {
        free(R_db.bptree_meta.leaf_addrs);
    }

    if (S_db.bptree_meta.leaf_addrs)
    {
        free(S_db.bptree_meta.leaf_addrs);
    }

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
    block_t *block;
    addr_t blk_addr;
    bptree_t R_bptree, S_bptree;

    bptree_init(&R_bptree, &(R_db->bptree_meta), buffer);

    blk_addr = R_HEAD_BLK_ADDR;
    for (int blk_num = 0; blk_num < R_BLK_NUM; blk_num++)
    {
        block = new_blk(buffer);
        while(block->tuple_num < TUPLES_PER_BLK)
        {
            gen_R(&(block->R_tuples[block->tuple_num]));
            bptree_insert(&R_bptree, block->R_tuples[block->tuple_num].A, blk_addr + block->tuple_num);
            block->tuple_num++;
        }

        if (blk_num < R_BLK_NUM - 1)
        {
            block->next_blk = next_blk_addr(blk_addr);
        }
        else
        {
            block->next_blk = 0;
        }
        save_blk(buffer, block, blk_addr, 1);
        blk_addr = next_blk_addr(blk_addr);
    }

    // bptree_print(&R_bptree);
    bptree_getmeta(&R_bptree, &(R_db->bptree_meta));
    bptree_free(&R_bptree);

    bptree_init(&S_bptree, &(S_db->bptree_meta), buffer);

    blk_addr = S_HEAD_BLK_ADDR;
    for (int blk_num = 0; blk_num < S_BLK_NUM; blk_num++)
    {
        block = new_blk(buffer);
        while(block->tuple_num < TUPLES_PER_BLK)
        {
            gen_S(&(block->S_tuples[block->tuple_num]));
            bptree_insert(&S_bptree, block->S_tuples[block->tuple_num].C, blk_addr + block->tuple_num);
            block->tuple_num++;
        }

        if (blk_num < S_BLK_NUM - 1)
        {
            block->next_blk = next_blk_addr(blk_addr);
        }
        else
        {
            block->next_blk = 0;
        }
        save_blk(buffer, block, blk_addr, 1);
        blk_addr = next_blk_addr(blk_addr);
    }

    // bptree_print(&S_bptree);
    bptree_getmeta(&S_bptree, &(S_db->bptree_meta));
    bptree_free(&S_bptree);
    free_blk(buffer, block);
    return 0;
}