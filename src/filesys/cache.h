#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

struct cache_line {
	uint8_t data[512];
	block_sector_t sector_num;
	bool dirty;
	bool valid;
}

struct cache_line * cache;



#endif