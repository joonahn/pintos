#include "vm/frame.h"
#include "vm/page.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include <debug.h>

void frame_table_init()
{
	int i;
	clock_hand = 0;
	frame_table = malloc(FRAME_TABLE_SIZE * sizeof(struct frame));
	for (i = 0; i < FRAME_TABLE_SIZE; ++i)
	{
		frame_table[i].use_bit = 0;
		frame_table[i].valid = 0;
	}
}

struct frame * frame_table_seek(uint32_t* _vaddr, uint32_t* _pagedir)
{
  int i;
  for (i = 0; i < FRAME_TABLE_SIZE; ++i)
  {
    if((frame_table[i].vaddr == _vaddr) && (frame_table[i].pagedir == _pagedir) && frame_table[i].valid)
    {
      return &frame_table[i];
    }
  }

  return NULL;
}


uint32_t* frame_get_vaddr(struct frame* f)
{
	return f->vaddr;
}

uint32_t* frame_get_pagedir(struct frame* f)
{
	return f->pagedir;
}

bool frame_get_use_bit(struct frame* f)
{
	return f->use_bit;
}

bool frame_get_valid (struct frame* f)
{
	return f->valid ;
}


void frame_set_vaddr (struct frame* f, uint8_t* vaddr)
{
	f->vaddr = vaddr;
}

void frame_set_pagedir (struct frame* f, uint32_t * pagedir)
{
	f->pagedir = pagedir;
}

void frame_set_use_bit (struct frame* f, bool use_bit)
{
	f->use_bit = use_bit;
}

void frame_set_valid  (struct frame* f, bool valid)
{
	f->valid  = valid ;
}

/** Returns frame pointer */
/** Implement Clock Algorithm here */
uint8_t *frame_alloc(uint32_t * upage)
{
	uint8_t * kpage = palloc_get_page(PAL_USER | PAL_ZERO);
	int i;
	printf("kpage is allocated in %p\n",kpage);
	/* In case of no physical frame left: frame eviction through clock algorithm! */
	if(kpage == NULL)
	{
		printf("---------------------------SWAPING!!!!!-------------------------------------\n");
		// for (i = 0; i < FRAME_TABLE_SIZE; ++i)
		// {
		// 	printf("%d-th pagedir in frame table : %p\n", i, frame_table[i].pagedir);
		// 	printf("%d-th vaddr in frame table : %p\n", i, frame_table[i].vaddr);
		// 	printf("%d-th valid in frame table : %p\n", i, frame_table[i].valid);
		// }
		clock_hand++;
		while(1)
		{
			if(frame_table[clock_hand].pagedir == NULL)
			{
				clock_hand++;
				continue;
			}
			if(pagedir_is_accessed(frame_table[clock_hand].pagedir, frame_table[clock_hand].vaddr))
				break;
			if(clock_hand == FRAME_TABLE_SIZE)
			{
				clock_hand = 0;
				for (i = 0; i < FRAME_TABLE_SIZE; ++i)
					pagedir_set_accessed(frame_table[clock_hand].pagedir, frame_table[clock_hand].vaddr, 0);
			}
			clock_hand++;
		}
		printf("clock algorithm done\n");
		//evict to swap
		uint32_t * pagedir_to_evict = frame_table[clock_hand].pagedir;
		uint32_t * vaddr_to_evict = frame_table[clock_hand].vaddr;
		//***********************************uint 32??? uint 8????????**********************
		ASSERT(swap_move_disk(pagedir_to_evict, vaddr_to_evict));
		printf("evict to swap done\n");
		//set evicted bit from supplement page table
		struct thread * t = get_thread_by_pagedir(pagedir_to_evict);
		ASSERT(t!=NULL);
		struct page* page_to_evict = page_lookup(vaddr_to_evict, t->sup_page_table);
		page_set_evict(page_to_evict);
		page_set_type(page_to_evict, PAGE_SWAP);
		printf("evicted bit setting done\n");
		//remove from frame

		frame_free(pagedir_get_page(pagedir_to_evict, vaddr_to_evict));
		pagedir_clear_page(pagedir_to_evict, vaddr_to_evict);

		printf("removing from the frame done\n");
		//allocate new frame
		kpage = palloc_get_page(PAL_USER | PAL_ZERO);
		printf("---------------------------SWAPING!!!!!-------------------------------------\n");
	}
	// printf("frame table entry index:%d",(vtop(kpage) - USER_BASE)>>22);
	// printf("frame table entry valid: %d\n",frame_table[(vtop(kpage) - USER_BASE)>>22].valid);

	//Set our frame table 
	printf("Let's see kpage value - %p\n",kpage);
	// ASSERT(!frame_table[(vtop(kpage) - USER_BASE)>>22].valid);
	frame_table[(vtop(kpage) - USER_BASE)>>12].vaddr = (uint32_t *)upage;
	frame_table[(vtop(kpage) - USER_BASE)>>12].valid = 1;
	frame_table[(vtop(kpage) - USER_BASE)>>12].pagedir = thread_current()->pagedir;
	// printf("Let's see kpage value - %p\n",kpage);
	// printf("%d-th pagedir in frame table : %p\n", (vtop(kpage) - USER_BASE)>>12, frame_table[(vtop(kpage) - USER_BASE)>>12].pagedir);
	// printf("%d-th vaddr in frame table : %p\n", (vtop(kpage) - USER_BASE)>>12, frame_table[(vtop(kpage) - USER_BASE)>>12].vaddr);
	// printf("%d-th valid in frame table : %p\n", (vtop(kpage) - USER_BASE)>>12, frame_table[(vtop(kpage) - USER_BASE)>>12].valid);

	return kpage;
}



void frame_free(uint32_t *kpage)
{
	palloc_free_page(kpage);
	frame_set_valid(&frame_table[(vtop(kpage) - USER_BASE)>>12], 0);
}