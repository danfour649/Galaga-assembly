#include "galaga.h"

#ifndef GALAGA_USE_ASM
#define GALAGA_USE_ASM 1
#endif

#if !GALAGA_USE_ASM

int rect_overlap(const Rect *a, const Rect *b)
{
    int a_right  = a->x + a->w;
    int a_bottom = a->y + a->h;
    int b_right  = b->x + b->w;
    int b_bottom = b->y + b->h;

    if (a->x >= b_right)  return 0;
    if (b->x >= a_right)  return 0;
    if (a->y >= b_bottom) return 0;
    if (b->y >= a_bottom) return 0;
    return 1;
}

#endif
