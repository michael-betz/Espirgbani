#ifndef PALETTE_H
#define PALETTE_H
#include <stdint.h>
#define P_SIZE 32

extern const uint32_t g_palettes[][P_SIZE];

const uint32_t *get_random_palette();

#endif
