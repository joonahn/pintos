#include "userprog/syscall.h"
#include "threads/thread.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include <list.h>
#include <hash.h>
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "threads/loader.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/fdmap.h"
#include "devices/input.h"
#include "vm/page.h"


#define PHYS_BASE ((void *) LOADER_PHYS_BASE)
#define get_virtual_addr(addr) \
        ((addr < PHYS_BASE) ? ((uint32_t *)pagedir_get_page(cur->pagedir, addr)) : (0))

struct lock *filelock = NULL;

static void syscall_handler (struct intr_frame *);

struct file * get_file(int _fd)
{
  struct thread* cur =  thread_current();
  struct list_elem *e;
  struct fdmap *f;
  int flag = 0;

  for (e = list_begin(&cur->fd_mapping_list);
    e != list_end(&cur->fd_mapping_list);
    e =  list_next(e))
  {
    f = list_entry(e, struct fdmap, fdmap_elem);
    if(f->fd == _fd)
    {
      flag = 1;
      break;
    }
  }
  if(flag)
    return f->fp;
  else
    return NULL;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  if(filelock == NULL)
  {
    filelock = (struct lock *)malloc(sizeof(struct lock));
    lock_init(filelock);
  }
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  uint32_t *arg1, *arg2, *arg3;
  struct thread *cur = thread_current ();
  int syscall_number, *syscall_number_ptr;

  syscall_number_ptr = (int*)get_virtual_addr(f->esp);
  arg1 = get_virtual_addr(f->esp + 4);
  arg2 = get_virtual_addr(f->esp + 8);
  arg3 = get_virtual_addr(f->esp + 12);

  //wrong user pointer
  if(!syscall_number_ptr)
    exit(-1);

  syscall_number = *syscall_number_ptr;
  
  //argument check
  if(!arg_check(syscall_number, arg1, arg2, arg3))
    exit(-1);

  switch(syscall_number)
  {
    // No argument
    case SYS_HALT:
    {
      halt();
      break;
    }

    // #of arg: 1
    case SYS_EXIT:
    {
      exit(*((int *)arg1));
      break;
    }
    case SYS_EXEC:
    {

      if(!get_virtual_addr(*((char **)arg1)))
        exit(-1);

      lock_acquire(filelock);
      f->eax = process_execute(*((char **)arg1));
      lock_release(filelock);
      // printf("EAX: %d",f->eax);
      break;
    }
    case SYS_WAIT:
    {
      f->eax = process_wait(*((int *)arg1));
      break;
    }
    case SYS_CLOSE:
    {
      lock_acquire(filelock);
      close(*((int*)arg1));
      lock_release(filelock);
      break;
    }
    case SYS_TELL:
    {
      f->eax = tell(*((int*)arg1));
      break;
    }
    case SYS_OPEN:
    {
      // lock_acquire(filelock);
      f->eax = open(*((char**) arg1));
      // lock_release(filelock);
      break;
    }
    case SYS_FILESIZE:
    {
      f->eax = filesize(*((int*)arg1));
      break;
    }
    case SYS_REMOVE:
    {
      f->eax = remove(*((char**) arg1));
      break;
    }
    case SYS_MUNMAP:
    {
      munmap(*(int *)arg1);
      break;
    }


    //#of arg : 2
    case SYS_SEEK:
    {
      seek(*((int*) arg1), *((unsigned *)arg2));
      break;
    }
    case SYS_CREATE:
    {
      f->eax = create(*((char**) arg1), *((unsigned *)arg2));
      break;
    }
    case SYS_MMAP:
    {
      f->eax = mmap(*(int*)arg1, *((char **)arg2));
      break;
    }


    //#of arg : 3
    case SYS_READ:
    {
      f->eax = read(*((int*)arg1), *((char**) arg2), *((unsigned *)arg3));
      break;
    }
    case SYS_WRITE:
    {
      f->eax = write(*((int*)arg1), *((char**) arg2), *((unsigned *)arg3));
      break;
    }
  }
  //process_exit();
  //thread_exit ();
}

void halt (void)
{
  shutdown_power_off();
}

void exit (int status)
{
  struct fdmap* map;
  struct list_elem * e;
  struct thread *cur = thread_current ();
  cur->exitstat = status;


  for (e = list_begin(&cur->fd_mapping_list);
        e != list_end(&cur->fd_mapping_list);
        e =  list_next(e))
  {

    map = list_entry(e, struct fdmap, fdmap_elem);
    list_remove(e);
    e = list_begin(&cur->fd_mapping_list);
    file_close(map->fp);
    free(map);
    if(!list_size(&cur->fd_mapping_list))
      break;
  }
  printf ("%s: exit(%d)\n", cur->process_name, status);
  thread_exit ();
}

int write (int _fd, const void *buffer, unsigned size)
{
  //exception handling
  struct thread *cur = thread_current ();
  if(get_virtual_addr(buffer) == 0)
    exit(-1);

  if(_fd==1)
  {
    putbuf(buffer, size);
    return size;
  }
  else
  {
    struct file * f;
    //struct inode * f_node;
    f = get_file(_fd);
    //f_node = file_get_inode (f);
    // printf("write, deny_num: %d\n", inode_deny_number(f_node));
    return (f!=NULL) ? file_write(f, buffer, size) : -1;
  }
}

bool create (const char *file, unsigned initial_size)
{
  struct thread *cur = thread_current ();

  if(file == NULL ||  !get_virtual_addr(file) || !strlen(file))
    exit(-1);


  return filesys_create(file, initial_size);
}

bool remove (const char *file)
{
  struct thread *cur = thread_current ();
  if(file == NULL ||  !get_virtual_addr(file) || !strlen(file))
    exit(-1);
  return filesys_remove(file);
}

int open (const char *file)
{
  int _fd = 2;
  struct thread *cur = thread_current ();
  

  if(file == NULL ||  !get_virtual_addr(file))
    exit(-1);
  if(!strlen(file))
    return -1;
  for(;;_fd++)
  {
    struct list_elem *e = NULL;
    struct fdmap *f = NULL;

    //locate unused fd number from 2
    // printf("list_begin : %p\n", list_begin(&cur->fd_mapping_list));
    // printf("list_end   : %p\n", list_end(&cur->fd_mapping_list));
    for (e = list_begin(&cur->fd_mapping_list);
      e != list_end(&cur->fd_mapping_list);
      e =  list_next(e))
    { 
      // printf("***************오빠char뽑았다\n");
      f = list_entry(e, struct fdmap, fdmap_elem);
      if(f->fd == _fd)
        break;
    }

    // if(f==NULL)
      // printf("**************f is null!\n");

    //Found!
    if(list_empty(&cur->fd_mapping_list) || f == NULL)
      break;
    if(e == list_end(&cur->fd_mapping_list) && f->fd != _fd)
      break;
  }
  // printf("breakbreakbreak\n");
  //map the file 
  struct fdmap * mapping = 
    (struct fdmap *)malloc(sizeof(struct fdmap));
  mapping->fd = _fd;
  lock_acquire(filelock);
  mapping->fp = filesys_open(file);
  lock_release(filelock);
  if(!mapping->fp)
  {
    free(mapping);
    return -1;
  }

  //Add to the fd mapping list of current thread
  list_push_back(&cur->fd_mapping_list, &mapping->fdmap_elem);
  // printf("open finished with size %d\n", list_size(&cur->fd_mapping_list));
  return _fd;
}

int filesize(int _fd)
{
  struct file * f;
  f = get_file(_fd);
  return (f!=NULL) ? file_length(f) : -1;
}

int read (int _fd, void *buffer, unsigned length)
{
  //exception handling
  struct thread *cur = thread_current ();
  // if(get_virtual_addr(buffer) == 0)
  // {
  //   printf("getvirtualaddr\n");
  //   exit(-1);
  // }
  if(buffer >= PHYS_BASE)
  {
    exit(-1);
  }


  if(_fd==0)
  {
    input_getc();
  }
  else
  {
    struct file * f;
    f = get_file(_fd);
    return (f != NULL) ? file_read(f, buffer, length) : -1;
  }
}

void seek (int _fd, unsigned position)
{
  //TODO: do we have to consider fd 0 or 1?????
  struct file *f;
  f = get_file(_fd);
  file_seek(f, position);
}

unsigned tell (int _fd)
{
  struct file *f;
  f = get_file(_fd);
  return (f != NULL) ? file_tell(f) : -1;
}

void close (int _fd)
{
  struct thread* cur =  thread_current();
  struct list_elem *e;
  struct fdmap *map;
  struct file *f;
  int flag = 0;

  f = get_file(_fd);
  if(!f)
    exit(-1);

  // printf("remove started\n");

  //remove from list
  for (e = list_begin(&cur->fd_mapping_list);
    e != list_end(&cur->fd_mapping_list);
    e =  list_next(e))
  {
    map = list_entry(e, struct fdmap, fdmap_elem);
    // printf("map : %p\n", map);
    if(map->fd == _fd)
    {
      list_remove(e);
      free(map);
      break;
    }
  }

  file_close(f);
}

mapid_t mmap(int _fd, void *addr)
{
  struct page *pte;
  struct file *f;
  int page_count = 0;
  int _filesize;
  int tmp_page_count = 0;
  int tmp_filesize;
  int mapid = 0;
  struct thread *cur = thread_current ();
  f = get_file(_fd);
  f = file_reopen(f);
  tmp_filesize = _filesize = filesize(_fd);

  //Exception handling
  if(_filesize == 0 || _fd == 0 || _fd == 1 || addr == 0)
    return -1;
  if(((int)addr) % PGSIZE != 0)
    return -1;

  //Virtual address overlapping avoidance
  while(tmp_filesize > 0)
  {
    size_t page_read_bytes = tmp_filesize < PGSIZE ? tmp_filesize : PGSIZE;
    
    if(page_lookup(addr + tmp_page_count * PGSIZE, cur->sup_page_table) != NULL)
      return -1;

    tmp_filesize -= page_read_bytes;
    tmp_page_count++;
  }

  //mapping between mapid_t and pte
  for(;;mapid++)
  {
    struct list_elem *e = NULL;
    struct mmap_table *m = NULL;

    for(e = list_begin(&cur->mmap_list);
        e!= list_end(&cur->mmap_list);
        e = list_next(e))
    {
      m = list_entry(e, struct mmap_table, mmap_elem);
      if(m->mapid == mapid)
        break;
    }

    //Found
    if(list_empty(&cur->mmap_list) || m == NULL)
      break;
    if(e == list_end(&cur->mmap_list) && m->mapid != mapid)
      break;
  }

  //map
  struct mmap_table *mapping = malloc(sizeof(struct mmap_table));
  mapping->mapid = mapid;
  mapping->pagedir = cur->pagedir;
  mapping->uaddr = addr;
  mapping->size = _filesize;
  mapping->file = f;

  list_push_back(&cur->mmap_list, &mapping->mmap_elem);


  while(_filesize > 0)
  {
    size_t page_read_bytes = _filesize < PGSIZE ? _filesize : PGSIZE;
    
    pte = malloc(sizeof(struct page));
    set_page(pte, (uint8_t *)addr + page_count * PGSIZE, f,
              page_count * PGSIZE, page_read_bytes, 1, 1, PAGE_FILE, 0, 1, 0);
    hash_insert(thread_current()->sup_page_table, get_hash_elem(pte));
    
    _filesize -= page_read_bytes;
    page_count++;
  }
  return mapid;
}

void munmap(mapid_t mapid)
{
  struct thread* cur =  thread_current();
  struct list_elem *e;
  struct mmap_table *map;
  struct file *f;
  int _filesize;
  int page_count = 0;

  for(e = list_begin(&cur->mmap_list);
      e!= list_end(&cur->mmap_list);
      e = list_next(e))
  {
    map = list_entry(e, struct mmap_table, mmap_elem);
    if(map->mapid == mapid)
    {
      //File reopen
      f = file_reopen(map->file);
      if(f != NULL)
      {
        _filesize = map->size;

        //Deallocate
        while(_filesize > 0)
        {
          size_t page_read_bytes = _filesize < PGSIZE ? _filesize : PGSIZE;
          struct page * page_to_unmap = page_lookup(map->uaddr + page_count * PGSIZE, 
                                                thread_current()->sup_page_table);

          //Dirty page write-back
          if(!page_get_evicted(page_to_unmap) &&
                      page_get_valid(page_to_unmap) && 
                      pagedir_is_dirty(map->pagedir, map->uaddr + page_count * PGSIZE))
          {
            file_seek(f, page_get_file_offset(page_to_unmap));
            file_write(page_get_file(page_to_unmap), 
                        pagedir_get_page(map->pagedir, map->uaddr + page_count * PGSIZE),
                        page_get_size(page_to_unmap));
          }

          //Frame dealloc
          if(!page_get_evicted(page_to_unmap))
          {
            frame_free(pagedir_get_page(map->pagedir, map->uaddr + page_count * PGSIZE));
            pagedir_clear_page(map->pagedir, map->uaddr + page_count * PGSIZE);
          }

          page_delete(page_to_unmap, thread_current()->sup_page_table);
          
          _filesize -= page_read_bytes;
          page_count++;
        }
      }

      //Deallocate


      list_remove(e);
      file_close(map->file);
      free(map);
      break;
    }
  }
}

int arg_check(int syscall_number,
    uint32_t * arg1, uint32_t * arg2, uint32_t * arg3)
{
  switch(syscall_number)
  {
    case SYS_HALT:
      return 1;
    // #of arg: 1
    case SYS_EXIT:
    case SYS_EXEC:
    case SYS_WAIT:
    case SYS_CLOSE:
    case SYS_TELL:
    case SYS_REMOVE:
    case SYS_OPEN:
    case SYS_FILESIZE:
    {
      if(!arg1)
        return 0;
    }


    //#of arg : 2
    case SYS_SEEK:
    case SYS_CREATE:
    {
      if(!(arg1 && arg2))
        return 0;
    }


    //#of arg : 3
    case SYS_READ:
    case SYS_WRITE:
    {
      if(!(arg1 && arg2 && arg3))
        return 0;
    }
  }
  return 1;
}