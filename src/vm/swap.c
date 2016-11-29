#include "vm/swap.h"
#include "threads/thread.h"
#include "vm/page.h"
#include <debug.h>


struct block *swap_device;
static struct bitmap *swap_map;

void swap_init(void)
{
	swap_device = block_get_role(BLOCK_SWAP);
	swap_map = bitmap_create(block_size(swap_device));
	printf("block size:%d\n", block_size(swap_device));
	printf("block type:%d\n", block_type(swap_device));
}


bool swap_move_disk(uint32_t* pd, uint8_t * uaddr)
{
	int i;
	block_sector_t sector = bitmap_scan_and_flip (swap_map, 0, 8, false);
	if (sector == BITMAP_ERROR)
		return 0;
	for (i = 0; i < 8; i++)
		block_write (swap_device, sector + i, pagedir_get_page(pd, uaddr + (BLOCK_SECTOR_SIZE * i)));
	//supplement page table evict bit set
	struct thread* t =  get_thread_by_pagedir(pd);
	ASSERT(t!=NULL);
	struct page* page_to_evict = page_lookup(uaddr, t->sup_page_table);
	page_set_swap_block(page_to_evict, sector);
	return 1;
}

bool swap_load(uint32_t* pd, uint8_t * uaddr)
{
	int i;
	struct thread* t = get_thread_by_pagedir(pd);
	ASSERT(t!=NULL);
	struct page* page_to_load = page_lookup(uaddr, t->sup_page_table);
	block_sector_t sector = page_get_swap_block(page_to_load);
	bitmap_set_multiple(swap_map, sector, 8, false);
	for(i = 0; i < 8; i++)
		block_read (swap_device, sector + i, pagedir_get_page(pd, uaddr + (BLOCK_SECTOR_SIZE * i)));
	return 1;
}