#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "threads/malloc.h"

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
	/* In case of no physical frame left: frame eviction through clock algorithm! */
	if(kpage == NULL)
	{
		// clock_hand++;
		// for(;pagedir_is_accessed(frame_table[clock_hand].pagedir, frame_table[clock_hand].vaddr); clock_hand++)
		// {
		// 	if(clock_hand == FRAME_TABLE_SIZE)
		// 	{
		// 		clock_hand = 0;
		// 		for (i = 0; i < FRAME_TABLE_SIZE; ++i)
		// 			pagedir_set_accessed(frame_table[clock_hand].pagedir, frame_table[clock_hand].vaddr, 0);
		// 	}
		// }
		//TODO:eviction and new allocation here




		exit(-1);
	}

	//Set our frame table 
	frame_table[(vtop(kpage) - USER_BASE)>>22].vaddr = (uint32_t *)upage;
	frame_table[(vtop(kpage) - USER_BASE)>>22].valid = 1;
	frame_table[(vtop(kpage) - USER_BASE)>>22].pagedir = thread_current()->pagedir;

	return kpage;
}



void frame_free(uint32_t *kpage)
{
	palloc_free_page(kpage);
	frame_set_valid(&frame_table[(vtop(kpage) - USER_BASE)>>22], 0);
}