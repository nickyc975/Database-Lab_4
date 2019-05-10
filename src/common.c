#include <time.h>
#include <stdlib.h>

#include "common.h"

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