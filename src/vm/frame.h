#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"
#include <stdint.h>
#define FRAME_TABLE_SIZE 384

struct frame *frame_table;

struct frame {
	uint32_t * vaddr;
	uint32_t * pagedir;
	bool use_bit;
	bool valid;
};

void frame_table_init(void);
struct frame* frame_table_seek(uint32_t * vaddr, uint32_t * pagedir);

uint32_t* frame_get_vaddr(struct frame*);
uint32_t* frame_get_pagedir(struct frame*);
bool frame_get_use_bit(struct frame*);
bool frame_get_valid (struct frame*);

void frame_set_vaddr(struct frame*, uint32_t*);
void frame_set_pagedir(struct frame*, uint32_t*);
void frame_set_use_bit(struct frame*, bool);
void frame_set_valid (struct frame*, bool);


#endif
