#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>

struct hash sup_page_table;

struct page
{
  //int swap_number;
  uint32_t *vaddr;
  int fd;
  int file_offset;
  bool valid;
  bool evicted;
  bool mmaped;
  bool prevent;

  struct hash_elem entryelem;
};

unsigned page_hash(const struct hash_elem *elem, void* aux);
bool page_less(const struct hash_elem *a, const struct hash_elem *b, void* aux);
struct page* page_lookup (const void* address);





#endif
