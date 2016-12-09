#include "filesys/cache.h"
#include "filesys/filesys.h"


void cache_init()
{
	int i;
	cache = malloc(sizeof(struct cache_line) * CACHE_SIZE);
	for(i=0; i<CACHE_SIZE; i++)
	{
		cache[i].valid = false;
	}
}

void dump_sector(block_sector_t sector)
{
	int i;
	char data[512];
	char line[64];
	block_read(fs_device, sector, data);
	for(i=0; i<8; i++)
	{

	}
}