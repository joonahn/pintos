#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>


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
  
  struct hash_elem entryelem;
};


struct hash * page_init(void);
unsigned page_hash(const struct hash_elem *elem, void* aux);
bool page_less(const struct hash_elem *a, const struct hash_elem *b, void* aux);
struct page* page_lookup (const void* address, struct hash * sup_page_table);
void set_page(struct page * pte, uint8_t *vaddr, struct file * file, int file_offset, int size, bool valid, bool evicted, enum PAGE_TYPE page_type, bool prevent, bool writable);
bool load_page(struct page * pte);
struct hash_elem * get_hash_elem(struct page * pte);
void grow_stack(uint8_t *vaddr);



#endif
