#include "threads/thread.h"
#include "vm/page.h"
#include <hash.h>

static bool install_page (void *upage, void *kpage, bool writable);

struct hash * page_init(void)
{
  struct hash * new = malloc(sizeof(struct hash));
  hash_init(new, page_hash, page_less, NULL);
  return new;
}

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

struct page* page_lookup (const void* address, struct hash * sup_page_table)
{
  struct page p;
  struct hash_elem* e;

  p.vaddr = address;
  e = hash_find(sup_page_table, &p.entryelem);

  return (e == NULL) ? NULL : hash_entry(e, struct page, entryelem);
}

void set_page(struct page * pte, uint8_t *vaddr, struct file * file,
              int file_offset, int size, bool valid,
              bool evicted, enum PAGE_TYPE page_type, bool prevent, bool writable)
{
  pte->vaddr = vaddr;
  pte->file = file;
  pte->file_offset = file_offset;
  pte->size = size;
  pte->valid = valid;
  pte->evicted = evicted;
  pte->page_type = page_type;
  pte->prevent = prevent;
  pte->writable = writable;
}

bool load_page(struct page * pte)
{
  uint8_t * kpage = frame_alloc(pte->vaddr);
  pte->prevent = 1;
  switch(pte->page_type)
  {
    case PAGE_EXEC:
    case PAGE_FILE:
    {
      //Lazy loading
      file_seek(pte->file, pte->file_offset);
      if(file_read(pte->file, kpage, pte->size)!=(pte->size))
      {
        frame_free(kpage);
        return false;
      }
      pte->evicted = 0;
      break;
    }
  }
  pte->prevent = 0;
  install_page(pte->vaddr, kpage, pte->writable);
  return true;
}

struct hash_elem * get_hash_elem(struct page * pte)
{
  return &(pte->entryelem);
}

void grow_stack(uint8_t *vaddr)
{
  uint8_t * kpage = frame_alloc(vaddr);
  struct page * pte = malloc(sizeof(struct page));

  set_page(pte, vaddr, NULL, 0,0,1,0,PAGE_SWAP, 0,1);
  install_page(vaddr, kpage, true);
}

  static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
      && pagedir_set_page (t->pagedir, upage, kpage, writable));
}


