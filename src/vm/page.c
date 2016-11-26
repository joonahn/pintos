#include "vm/page.h"

unsigned page_hash(const struct hash_elem *elem, void* aux)
{
  const struct page *p = hash_entry(elem, struct page, entryelem);
  return hash_bytes (&p->vaddr, sizeof(p->vaddr));
}

bool page_less(const struct hash_elem* a_, const struct hash_elem* b_,
              void* aux)
{
  const struct page *a = hash_entry(a_, struct page, entryelem);
  const struct page *b = hash_entry(b_, struct page, entryelem);

  return a->vaddr < b->vaddr;
}

struct page* page_lookup (const void* address)
{
  struct page p;
  struct hash_elem* e;

  p.vaddr = address;
  e = hash_find(&sup_page_table, &p.entryelem);

  return (e == NULL) ? NULL : hash_entry(e, struct page, entryelem);
}
