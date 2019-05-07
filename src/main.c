#include "common.h"
#include "../lib/libextmem.h"

int main(int argc, char *argv[])
{
    Buffer _buffer;
    Buffer *buffer = &_buffer;
    initBuffer(520, 64, buffer);

    Block *block = (Block *)getNewBlockInBuffer(buffer);

    for (int blk_num = 0; blk_num < 16; blk_num++)
    {
        for (int i = 0; i < TUPLES_PER_BLK; i++)
        {
            gen_R(&(block->R_data[i]));
        }
        block->next_blk = blk_num + 1;
        writeBlockToDisk((char *)block, blk_num, buffer);
        block = (Block *)getNewBlockInBuffer(buffer);
    }

    return 0;
}