#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <bitmap.h>
#include "devices/block.h"

enum PAGE_TYPE {
	PAGE_EXEC,
	PAGE_FILE,
	PAGE_SWAP,
	PAGE_ZERO
};

struct page
{
  //int swap_number;
  uint8_t *vaddr;
  struct file * file;
  int file_offset;
  int size;
  bool valid;
  bool evicted;
  enum PAGE_TYPE page_type;
  bool prevent;
  bool writable;
  block_sector_t swap_block;
  
  struct hash_elem entryelem;
};


struct hash * page_init(void);
unsigned page_hash(const struct hash_elem *elem, void* aux);
bool page_less(const struct hash_elem *a, const struct hash_elem *b, void* aux);
//for hash_destroy
void page_destroy(const struct hash_elem *e, void *aux);
//for hash_elem_delete
void page_delete(const struct page * pte, struct hash * sup_page_table);

struct page* page_lookup (const void* address, struct hash * sup_page_table);
void set_page(struct page * pte, uint8_t *vaddr, struct file * file, int file_offset, int size, bool valid, bool evicted, enum PAGE_TYPE page_type, bool prevent, bool writable, block_sector_t swap_block);
void page_set_evict(struct page * pte);
void page_set_swap_block(struct page* pte, block_sector_t sector);
void page_set_type(struct page* pte, enum PAGE_TYPE page_type);
int page_get_type(struct page * pte);
int page_get_size(struct page * pte);
bool page_get_evicted(struct page * pte);
bool page_get_valid(struct page * pte);
int page_get_file_offset(struct page * pte);
bool page_is_prevent(struct page *pte);
struct file * page_get_file(struct page * pte);
block_sector_t page_get_swap_block(struct page* pte);
bool load_page(struct page * pte);
struct hash_elem * get_hash_elem(struct page * pte);
void grow_stack(uint8_t *vaddr);



#endif
