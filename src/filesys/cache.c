#include "filesys/cache.h"


void cache_lru_use(block_sector_t sector)
{
	struct list_elem * e;
	struct LRU_queue_element * lru_element;
	for(e = list_begin(&LRU_queue);
		e!= list_end(&LRU_queue);
		e = list_next(e))
	{
		lru_element = list_entry(e, struct LRU_queue_element, lruelem);
		if(lru_element->sector == sector)
		{
			list_remove(e);
			list_push_back(&LRU_queue, e);
			return;
		}
	}
}

void cache_lru_insert(block_sector_t sector, int cache_line)
{
	struct LRU_queue_element * lru_element = 
		(struct LRU_queue_element *)malloc(sizeof(struct LRU_queue_element));
	lru_element->sector = sector;
	lru_element->cache_line = cache_line;
	list_push_back(&LRU_queue, &lru_element->lruelem);
}

void cache_lru_remove(block_sector_t sector)
{
	struct list_elem * e;
	struct LRU_queue_element * lru_element;
	for(e = list_begin(&LRU_queue);
		e!= list_end(&LRU_queue);
		e = list_next(e))
	{
		lru_element = list_entry(e, struct LRU_queue_element, lruelem);
		if(lru_element->sector == sector)
		{
			list_remove(e);
			free(lru_element);
			return;
		}
	}
}

static inline size_t cache_lru_count(void)
{
	return list_size(&LRU_queue);
}

int cache_find_victim()
{
	// is cache full?
	ASSERT(list_size(&LRU_queue) == CACHE_SIZE);

	int victim;
	struct list_elem * e;
	struct LRU_queue_element * lru_element;
	e = list_pop_front(&LRU_queue);
	lru_element = list_entry(e, struct LRU_queue_element, lruelem);
	victim = lru_element->cache_line;
	free(lru_element);
	return victim;
}

void cache_init()
{
	int i;
	cache = malloc(sizeof(struct cache_line) * CACHE_SIZE);
	for(i=0; i<CACHE_SIZE; i++)
	{
		cache[i].valid = false;
		cache[i].dirty = false;
	}
	list_init(&LRU_queue);
}

void dump_sector(block_sector_t sector)
{
	int i;
	char data[512];
	char line[64];
	block_read(fs_device, sector, data);
	printf("Data of sector %u\n", sector);
	for(i=0; i<8; i++)
	{
		memcpy(line, data + 64 * i, 64);
		hex_dump(0, line, 64, true);
	}
}

void cache_block_read(block_sector_t sector, void * buffer)
{
	int i;
	for (i = 0; i < CACHE_SIZE; ++i)
	{
		if(cache[i].valid == true && 
			cache[i].sector == sector)
		{
			//Cache hit
			memcpy(buffer, cache[i].data, CACHE_DATA_SIZE);
			cache_lru_use(sector);
			return;
		}
	}

	//Cache miss - eviction needed
	if(cache_lru_count() == CACHE_SIZE)
	{
		int victim = cache_find_victim();

		//Write back
		if(cache[victim].dirty)
			block_write(fs_device, cache[victim].sector, cache[victim].data);

		cache_lru_remove(cache[victim].sector);
		cache_lru_insert(sector, victim);

		//Change data
		cache[victim].sector = sector;
		cache[victim].dirty = false;
		block_read(fs_device, sector, cache[victim].data);

		memcpy(buffer, cache[victim].data, CACHE_DATA_SIZE);
		return;
	}
	//Cache miss - no eviction needed
	else
	{
		for (i = 0; i < CACHE_SIZE; ++i)
		{
			if(cache[i].valid == false)
			{
				cache[i].valid = true;
				cache[i].dirty = false;
				cache[i].sector = sector;
				block_read(fs_device, sector, cache[i].data);
				memcpy(buffer, cache[i].data, CACHE_DATA_SIZE);
				cache_lru_insert(sector, i);
				return;
			}
		}
	}
}

void cache_block_write(block_sector_t sector, void * buffer)
{
	int i;

	for (i = 0; i < CACHE_SIZE; ++i)
	{
		if(cache[i].valid == true && 
			cache[i].sector == sector)
		{
			//Cache hit
			cache[i].dirty = true;
			memcpy(cache[i].data, buffer, CACHE_DATA_SIZE);
			cache_lru_use(sector);
			return;
		}
	}

	//Cache miss - eviction needed
	if(cache_lru_count() == CACHE_SIZE)
	{
		int victim = cache_find_victim();

		//Write back
		if(cache[victim].dirty)
			block_write(fs_device, cache[victim].sector, cache[victim].data);

		cache_lru_remove(cache[victim].sector);
		cache_lru_insert(sector, victim);

		//Change data
		cache[victim].sector = sector;
		cache[victim].dirty = true;

		memcpy(cache[victim].data, buffer, CACHE_DATA_SIZE);

		return;
	}

	//Cache miss - no eviction needed
	else
	{
		for (i = 0; i < CACHE_SIZE; ++i)
		{
			if(cache[i].valid == false)
			{
				cache[i].valid = true;
				cache[i].dirty = true;
				cache[i].sector = sector;
				memcpy(cache[i].data, buffer, CACHE_DATA_SIZE);
				cache_lru_insert(sector, i);
				return;
			}
		}
	}
}

void cache_flush(void)
{
	int i;
	for (i = 0; i < CACHE_SIZE; ++i)
	{
		if(cache[i].valid == true && 
			cache[i].dirty == true)
		{
			block_write(fs_device, cache[i].sector, cache[i].data);
			cache[i].dirty = false;
		}
	}
}