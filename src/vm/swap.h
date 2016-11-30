#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <bitmap.h>
#include "devices/block.h"


void swap_init(void);

bool swap_move_disk(uint32_t* pd, uint8_t * uaddr);
bool swap_load(uint32_t* pd, uint8_t * uaddr, uint8_t *kpage);
void swap_delete(uint32_t *pd, uint8_t * uaddr);

#endif