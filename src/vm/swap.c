#include "vm/swap.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "devices/timer.h"
#include "vm/page.h"
#include <debug.h>


struct block *swap_device;
static struct bitmap *swap_map;
struct semaphore swap_sema;
struct semaphore block_sema;

void swap_init(void)
{
	swap_device = block_get_role(BLOCK_SWAP);
	swap_map = bitmap_create(block_size(swap_device));
	sema_init(&swap_sema,1);
}


bool swap_move_disk(uint32_t* pd, uint8_t * uaddr)
{
	int i;
	sema_down(&swap_sema);
	timer_msleep(20);
	block_sector_t sector = bitmap_scan_and_flip (swap_map, 0, 8, false);
	ASSERT(sector != BITMAP_ERROR);
	for (i = 0; i < 8; i++)
		block_write (swap_device, sector + i, pagedir_get_page(pd, uaddr + (BLOCK_SECTOR_SIZE * i)));
	//supplement page table evict bit set
	struct thread* t =  get_thread_by_pagedir(pd);
	ASSERT(t!=NULL);
	struct page* page_to_evict = page_lookup(uaddr, t->sup_page_table);
	page_set_swap_block(page_to_evict, sector);
	sema_up(&swap_sema);
	return 1;
}

bool swap_load(uint32_t* pd, uint8_t * uaddr, uint8_t *kpage)
{
	int i;
	struct thread* t;
	sema_down(&swap_sema);
	timer_msleep(20);
	// printf("thread %d got swap sema!-swap_load()\n", thread_current()->tid);
	t = get_thread_by_pagedir(pd);
	ASSERT(t!=NULL);
	struct page* page_to_load = page_lookup(uaddr, t->sup_page_table);
	block_sector_t sector = page_get_swap_block(page_to_load);
	bitmap_set_multiple(swap_map, sector, 8, false);
	for(i = 0; i < 8; i++)
	{
		block_read (swap_device, sector + i, kpage + BLOCK_SECTOR_SIZE * i);
	}
	sema_up(&swap_sema);
	// printf("thread %d release swap sema!-swap_load()\n", thread_current()->tid);
	return 1;
}

void swap_delete(uint32_t *pd, uint8_t * uaddr)
{
	int i;
	struct thread* t;
	sema_down(&swap_sema);
	timer_msleep(20);
	// printf("thread %d got swap sema!-swap_delete()\n", thread_current()->tid);
	t = get_thread_by_pagedir(pd);
	ASSERT(t!=NULL);
	struct page* page_to_load = page_lookup(uaddr, t->sup_page_table);
	// printf("uaddr : %p, sup_page_table:%p \n", uaddr, t->sup_page_table);
	ASSERT(page_to_load!=NULL);
	block_sector_t sector = page_get_swap_block(page_to_load);
	bitmap_set_multiple(swap_map, sector, 8, false);
	sema_up(&swap_sema);
	// printf("thread %d release swap sema!-swap_delete()\n", thread_current()->tid);
}