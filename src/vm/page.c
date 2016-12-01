#include "threads/thread.h"
#include "vm/page.h"
#include "vm/frame.h"
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

void page_destroy(const struct hash_elem *e, void *aux)
{
  const struct page *pte = hash_entry(e, struct page, entryelem);
  free(pte);
}

void page_delete(const struct page * pte, struct hash * sup_page_table)
{
  hash_delete(sup_page_table, &pte->entryelem);
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
              bool evicted, enum PAGE_TYPE page_type, bool prevent, bool writable, block_sector_t swap_block)
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
  pte->swap_block = swap_block;
}

void page_set_evict(struct page * pte)
{
  pte->evicted = 1;
}

void page_set_swap_block(struct page* pte, block_sector_t sector)
{
  pte->swap_block = sector;
}

void page_set_type(struct page* pte, enum PAGE_TYPE page_type)
{
  pte->page_type = page_type;
}

int page_get_type(struct page * pte)
{
  return pte->page_type;
}

block_sector_t page_get_swap_block(struct page* pte)
{
  return pte->swap_block;
}

struct file * page_get_file(struct page * pte)
{
  return pte->file;
}

int page_get_size(struct page * pte)
{
  return pte->size;
}

int page_get_file_offset(struct page * pte)
{
  return pte->file_offset;
}

bool page_get_evicted(struct page * pte)
{
  return pte->evicted;
}
bool page_get_valid(struct page * pte)
{
  return pte->valid;
}

bool page_is_prevent(struct page *pte)
{
  return pte->prevent;
}

bool load_page(struct page * pte)
{
  pte->prevent = 1;
  uint8_t * kpage = frame_alloc(pte->vaddr);
  // printf("right before page type switch\n");
  switch(pte->page_type)
  {
    case PAGE_EXEC:
    case PAGE_FILE:
    {
      //Lazy loading
      file_seek(pte->file, pte->file_offset);
      if(file_read(pte->file, kpage, pte->size)!=(pte->size))
      {
        sema_down(&frame_sema);
        frame_free(kpage);
        sema_up(&frame_sema);
        return false;
      }
      pte->evicted = 0;
      break;
    }
    case PAGE_SWAP:
    {
      // printf("is evicted:%d\n",pte->evicted);
      if(pte->evicted)
      {
        //Load from swap disk
        // printf("swap_load( %p, %p )\n", thread_current()->pagedir, pte->vaddr );
        // printf("kpage : %p\n", kpage);
        swap_load(thread_current()->pagedir, pte->vaddr, kpage);
        pte->evicted = 0;
      }
      break;
    }
  }
  // printf("right after page type switch\n");
  install_page(pte->vaddr, kpage, pte->writable);
  pte->prevent = 0;
  // printf("right after installing page\n");
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

  set_page(pte, vaddr, NULL, 0,0,1,0,PAGE_SWAP,1,1,0);
  install_page(vaddr, kpage, true);
  pte->prevent = 0;
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


