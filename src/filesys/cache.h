#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H
#define CACHE_SIZE 64
#define CACHE_DATA_SIZE 512 //sector size

#include "devices/block.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include <string.h>
#include <console.h>
#include <debug.h>
#include <stdio.h>
#include <stdbool.h>
#include <list.h>

struct cache_line {
	uint8_t data[CACHE_DATA_SIZE];
	block_sector_t sector;
	bool dirty;
	bool valid;
};

struct cache_line * cache;

struct LRU_queue_element {
	struct list_elem lruelem;
	int cache_line;
	block_sector_t sector;
};

struct list LRU_queue;

// Functions for LRU queue
void cache_lru_use(block_sector_t sector);
void cache_lru_insert(block_sector_t sector, int cache_line);
void cache_lru_remove(block_sector_t sector);
static inline size_t cache_lru_count(void);
int cache_find_victim(void);

void cache_init(void);
void cache_flush(void);
void dump_sector(block_sector_t sector);

void cache_block_read(block_sector_t sector, void * buffer);

void cache_block_write(block_sector_t sector, void * buffer);

#endif