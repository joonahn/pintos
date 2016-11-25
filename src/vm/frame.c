#include "vm/frame.h"

void frame_table_init()
{
	int i;
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


void frame_set_vaddr (struct frame* f, uint32_t* vaddr)
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
