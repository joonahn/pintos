#include "swap.h"


struct block *swap_device;
static struct bitmap *swap_map;

void swap_init(void)
{
	swap_device = block_get_role(BLOCK_SWAP);
	printf("block size:%d\n", block_size(swap_device));
	printf("block type:%d\n", block_type(swap_device));

}


bool swap_move_disk(uint8_t * uaddr)
{
	int i;
	block_sector_t sector = bitmap_scan_and_flip (swap_map, 0, 8, false);
	if (sector == BITMAP_ERROR)
		return 0;
	for (i = 0; i < 8; i++)
		block_write (swap_device, sector + i, uaddr + (BLOCK_SECTOR_SIZE * i));
	return 1;
}

bool swap_load(uint8_t * uaddr)
{

}