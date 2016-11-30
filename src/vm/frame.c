#include "vm/frame.h"
#include "vm/page.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include <debug.h>



void frame_table_init()
{
	int i;
	clock_hand = 0;
	sema_init(&frame_sema, 1);
	frame_table = malloc(FRAME_TABLE_SIZE * sizeof(struct frame));
	sema_down(&frame_sema);
	for (i = 0; i < FRAME_TABLE_SIZE; ++i)
	{
		frame_table[i].use_bit = 0;
		frame_table[i].valid = 0;
	}
	sema_up(&frame_sema);
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
	uint8_t * kpage;
	int i;

	sema_down(&frame_sema);
	// printf("thread %d got frame sema!-frame_alloc()\n", thread_current()->tid);

	kpage = palloc_get_page(PAL_USER | PAL_ZERO);
	/* In case of no physical frame left:
	** frame eviction through clock algorithm! */
	if(kpage == NULL)
	{
		// printf("before clock algorithm\n");
		clock_hand++;
		while(1)
		{
			// Clock hand has rotated one cycle
			if(clock_hand >= FRAME_TABLE_SIZE)
			{
				clock_hand = 0;
				// Set all page's accessed bits zero
				for (i = 0; i < FRAME_TABLE_SIZE; ++i)
				{
					if(frame_table[i].valid != 0)
						pagedir_set_accessed(frame_table[i].pagedir, frame_table[i].vaddr, 0);
				}

			}

			// Increase clock hand if page is not valid
			if(frame_table[clock_hand].pagedir == NULL || frame_table[clock_hand].valid == 0)
			{
				clock_hand++;
				continue;
			}

			// Victim page found
			if(!pagedir_is_accessed(frame_table[clock_hand].pagedir, frame_table[clock_hand].vaddr))
				break;

			clock_hand++;
		}

		//evict to swap
		uint32_t * pagedir_to_evict = frame_table[clock_hand].pagedir;
		uint32_t * vaddr_to_evict = frame_table[clock_hand].vaddr;

		//set evicted bit from supplement page table
		struct thread * t = get_thread_by_pagedir(pagedir_to_evict);
		ASSERT(t!=NULL);
		struct page* page_to_evict = page_lookup(vaddr_to_evict, t->sup_page_table);
		page_set_evict(page_to_evict);
		


		switch(page_get_type(page_to_evict))
		{
			case PAGE_FILE:
			{
				if(pagedir_is_dirty(pagedir_to_evict, vaddr_to_evict))
				{
					file_seek(page_get_file(page_to_evict), page_get_file_offset(page_to_evict));
					file_write(page_get_file(page_to_evict), 
								pagedir_get_page(pagedir_to_evict, vaddr_to_evict),
								page_get_size(page_to_evict));
				}
				break;
			}
			default:
			{
				ASSERT(swap_move_disk(pagedir_to_evict, vaddr_to_evict));
				page_set_type(page_to_evict, PAGE_SWAP);
				break;
			}
		}
		// printf("before setting evicted bit\n");
		// printf("before removing to frame");
		//remove from frame
		frame_free(pagedir_get_page(pagedir_to_evict, vaddr_to_evict));
		pagedir_clear_page(pagedir_to_evict, vaddr_to_evict);
		// printf("before allocating new frame\n");
		//allocate new frame
		kpage = palloc_get_page(PAL_USER | PAL_ZERO);
		ASSERT(kpage != NULL);
	}
	// printf("before setting frame table\n");
	//Set our frame table 
	frame_table[(vtop(kpage) - USER_BASE)>>12].vaddr = (uint32_t *)upage;
	frame_table[(vtop(kpage) - USER_BASE)>>12].valid = 1;
	frame_table[(vtop(kpage) - USER_BASE)>>12].pagedir = thread_current()->pagedir;
	sema_up(&frame_sema);
	// printf("thread %d release frame sema!-frame_alloc()\n", thread_current()->tid);
	return kpage;
}



void frame_free(uint32_t *kpage)
{
	palloc_free_page(kpage);
	frame_set_valid(&frame_table[(vtop(kpage) - USER_BASE)>>12], 0);
}