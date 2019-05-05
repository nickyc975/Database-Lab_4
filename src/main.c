#include "common.h"
#include "../lib/extmem.h"

int main(int argc, char *argv[])
{
    Buffer _buffer;
    Buffer *buffer = &_buffer;
    initBuffer(520, 64, buffer);
    gen_data(buffer);

    Block *block = (Block *)getNewBlockInBuffer(buffer);

    block->R_data[0].A = 0;

    return 0;
}