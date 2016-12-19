#include "userprog/syscall.h"
#include "threads/thread.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include <list.h>
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "threads/loader.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/fdmap.h"
#include "filesys/directory.h"
#include "devices/input.h"


#define PHYS_BASE ((void *) LOADER_PHYS_BASE)
#define get_virtual_addr(addr) \
        ((addr < PHYS_BASE) ? ((uint32_t *)pagedir_get_page(cur->pagedir, addr)) : (0))

struct lock *filelock = NULL;

static void syscall_handler (struct intr_frame *);

static struct file * get_file(int _fd)
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
    case SYS_CHDIR:
    {
      f->eax = chdir(*((char **)arg1));
      break;
    }
    case SYS_MKDIR:
    {
      f->eax = mkdir(*((char **)arg1));
      break;
    }
    case SYS_ISDIR:
    {
      f->eax = isdir(*((int*)arg1));
      break;
    }
    case SYS_INUMBER:
    {
      f->eax = inumber(*((int*)arg1));
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
    case SYS_READDIR:
    {
      f->eax = readdir(*((int *) arg1), *((char **)arg2));
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

//TODO: free fd-mapping 
void exit (int status)
{
  struct fdmap* map;
  struct list_elem * e;
  struct thread *cur = thread_current ();
  // printf("exit called! with size %d", list_size(&cur->fd_mapping_list));
  cur->exitstat = status;

  // printf("list_begin : %p\n",  list_begin(&cur->fd_mapping_list));
  // printf("list_end : %p\n",  list_end(&cur->fd_mapping_list));

  for (e = list_begin(&cur->fd_mapping_list);
        e != list_end(&cur->fd_mapping_list);
        e =  list_next(e))
  {

    // printf("list_entry : %p\n",  list_entry(e, struct fdmap, fdmap_elem));
    map = list_entry(e, struct fdmap, fdmap_elem);
    list_remove(e);
    e = list_begin(&cur->fd_mapping_list);
    file_close(map->fp);/////////////////////////////////////////
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
    if(filesys_is_directory(f))
      return -1;
    return (f!=NULL) ? file_write(f, buffer, size) : -1;
  }
}

bool create (const char *file, unsigned initial_size)
{
  struct thread *cur = thread_current ();

  if(file == NULL ||  !get_virtual_addr(file) || !strlen(file))
    exit(-1);

  if(file[0] == '/')
  {
    //Absolute path
    if(dir_is_depth_is_too_deep(file))
    {
      return false;
    }
    return filesys_abs_create(file, initial_size);
  }
  else
  {
    //relative path
    bool success;
    char * abs_path;
    filesys_change_rel_to_abs(file, cur->curpath, &abs_path);

    if(dir_is_depth_is_too_deep(abs_path))
    {
      return false;
    }
    success = filesys_abs_create(abs_path, initial_size);
    free(abs_path);
    return success;
  }

  //not reachable
  return false;
}

bool remove (const char *file)
{
  struct thread *cur = thread_current ();
  if(file == NULL ||  !get_virtual_addr(file) || !strlen(file))
    exit(-1);

  if(file[0] == '/')
  {
    //Absolute path
    if(dir_is_same(cur->curpath, file))
      return false;
    return filesys_abs_remove(file);
  }
  else
  {
    //relative path
    bool success;
    char * abs_path;
    filesys_change_rel_to_abs(file, cur->curpath, &abs_path);

    if(dir_is_same(cur->curpath, abs_path))
      return false;

    success = filesys_abs_remove(abs_path);
    free(abs_path);
    return success;
  }
  //not reached
  return false;
  // return filesys_remove(file);
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

    if(f==NULL)
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
  //mapping->fp = filesys_open(file);

  if(file[0] == '/')
  {
    //Absolute path
    lock_acquire(filelock);
    mapping->fp = filesys_abs_open(file);
    lock_release(filelock);

  }
  else
  {
    //relative path
    char * abs_path;
    filesys_change_rel_to_abs(file, cur->curpath, &abs_path);
    lock_acquire(filelock);
    // printf("cur_path : %s\n", cur->curpath);
    // printf("abs_path : %s\n", abs_path);
    mapping->fp = filesys_abs_open(abs_path);
    lock_release(filelock);
    free(abs_path);
  }

  
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
  if(get_virtual_addr(buffer) == 0)
    exit(-1);

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


bool chdir (const char *dir)
{
  //check relative path or absolute path
  if(strlen(dir)==0)
    return true;
  if(dir[0] == '/')
  {
    //abs path
    struct dir * struct_dir;
    struct inode * inode;
    char filename[NAME_MAX + 1];

    //Root
    if(strcmp(dir, "/")==0)
    {
      free(thread_current()->curpath);
      thread_current()->curpath = (char *)malloc(2);
      memcpy(thread_current()->curpath, "/", 2);
      return true;
    }

    //Not Root, but absolute path
    struct_dir = dir_open_abs(dir, filename, NULL);
    if(struct_dir == NULL)
      return false;
    if(dir_lookup_dir(struct_dir, filename, &inode)==false)
    {
      inode_close(inode);
      return false;
    }
    else
    {
      //dir is a valid directory
      struct thread * cur = thread_current();
      inode_close(inode);
      free(cur->curpath);
      if(dir[strlen(dir)-1] == '/')
      {
        cur->curpath = (char *)malloc(strlen(dir) + 1);
        memcpy(cur->curpath, dir, strlen(dir) + 1);
      }
      else
      {
        cur->curpath = (char *)malloc(strlen(dir) + 2);
        memcpy(cur->curpath, dir, strlen(dir));
        (cur->curpath)[strlen(dir)] = '/';
        (cur->curpath)[strlen(dir) + 1] = 0;
      }
      return true;
    }
  }
  else
  {

    //abs path
    struct dir * struct_dir;
    struct inode * inode;
    char filename[NAME_MAX + 1];
    char * abs_path; 
    struct thread *cur;

    cur = thread_current();
    filesys_change_rel_to_abs(dir, cur->curpath, &abs_path);

    //Not Root, but absolute path
    struct_dir = dir_open_abs(abs_path, filename, NULL);
    if(struct_dir == NULL)
      return false;

    if(dir_lookup_dir(struct_dir, filename, &inode)==false)
    {
      inode_close(inode);
      free(abs_path);
      dir_close(struct_dir);
      return false;
    }
    else
    {
      //dir is a valid directory
      struct thread * cur = thread_current();
      inode_close(inode);
      free(cur->curpath);
      if(abs_path[strlen(dir)-1] == '/')
      {
        cur->curpath = (char *)malloc(strlen(abs_path) + 1);
        memcpy(cur->curpath, abs_path, strlen(abs_path) + 1);
      }
      else
      {
        cur->curpath = (char *)malloc(strlen(abs_path) + 2);
        memcpy(cur->curpath, abs_path, strlen(abs_path));
        (cur->curpath)[strlen(abs_path)] = '/';
        (cur->curpath)[strlen(abs_path) + 1] = 0;
      }
      dir_close(struct_dir);
      free(abs_path);
      return true;
    }
  }
  // {
  //   //relative path
  //   int depth = -1, i, length;
  //   struct dir * struct_dir = NULL;
  //   struct inode *inode = NULL;
  //   char *str, *token, *save_ptr;

  //   //Open Folders
  //   length = strlen(dir);
  //   str = malloc(length + 1);
  //   memcpy(str, dir, length + 1);
  //   struct_dir = dir_open_abs_dir (thread_current()->curpath);


  //   for (token = strtok_r (str, "/", &save_ptr); token != NULL;
  //        token = strtok_r (NULL, "/", &save_ptr))
  //   {
  //     //Is next directory exist?
  //     if(struct_dir!= NULL &&
  //       dir_lookup_dir(struct_dir, token, &inode))
  //     {
  //       inode_close(inode);
  //       dir_close(struct_dir);
  //       struct_dir = dir_open(inode);
  //     }
  //     // Next directory doesn't exist
  //     else
  //     {
  //       inode_close(inode);
  //       dir_close(struct_dir);
  //       free(str);
  //       return false;
  //     }
  //   }



  //   //Now, dir is the destination directory


  //   if(struct_dir == NULL)
  //   {
  //     free(str);
  //     return false;
  //   }
  //   else
  //   {
  //     char * tmp;
  //     char * curpath;
  //     //relative path is validated
  //     dir_close(struct_dir);
  //     free(str);

  //     str = malloc(length + 1);
  //     memcpy(str, dir, length + 1);

  //     for (token = strtok_r (str, "/", &save_ptr); token != NULL;
  //          token = strtok_r (NULL, "/", &save_ptr))
  //     {
  //       if(strcmp(".", token) == 0)
  //         continue;

  //       if(strcmp("..", token) == 0)
  //       {
  //         tmp = thread_current()->curpath;
  //         if(strcmp(tmp, "/") == 0)
  //           continue;
  //         for(i = strlen(tmp)-1; i >0; i--)
  //         {
  //           if(tmp[i-1] == '/')
  //           {
  //             tmp[i] = 0;
  //             break;
  //           }
  //         }
  //         continue;
  //       }

  //       curpath = thread_current()->curpath;
  //       tmp = (char *)malloc(strlen(curpath) + strlen(token) + 2);
  //       memcpy(tmp, curpath, strlen(curpath));
  //       memcpy(tmp + strlen(curpath), token, strlen(token));
  //       memcpy(tmp + strlen(curpath) + strlen(token), "/", 2);
  //       free(curpath);
  //       thread_current()->curpath = tmp;
  //     }

  //     free(str);
  //     return true;
  //   }

  // }
}

bool mkdir(const char *dir)
{
  //check relative path or absolute path
  if(strlen(dir)==0)
    return false;
  if(dir[0] == '/')
  {
    //abs path
    struct dir * struct_dir;
    struct inode * inode;
    char filename[NAME_MAX + 1];
    struct_dir = dir_open_abs(dir, filename, NULL);
    if(struct_dir == NULL)
      return false;
    if(dir_lookup_dir(struct_dir, filename, &inode))
    {
      inode_close(inode);
      dir_close(struct_dir);
      return false;
    }
    else
    {
      bool success;
      inode_close(inode);
      success = (dir_add_dir_with_allocate(struct_dir, filename));

      dir_close(struct_dir);
      return success;
    }
  }


  else
  {
    //relative path
    int depth = -1, i, length;
    struct dir * struct_dir = NULL;
    struct inode *inode = NULL;
    char *str, *token, *save_ptr;

    //Open Folders
    length = strlen(dir);
    str = malloc(length + 1);
    memcpy(str, dir, length + 1);


    //Calculate depth
    for (token = strtok_r (str, "/", &save_ptr); token != NULL;
         token = strtok_r (NULL, "/", &save_ptr))
    {
      depth ++;
    }

    free(str);
    str = malloc(length + 1);
    memcpy(str, dir, length + 1);
    struct_dir = dir_open_abs_dir (thread_current()->curpath);

    for ((token = strtok_r (str, "/", &save_ptr)), i = 0; i < depth;
         (token = strtok_r (NULL, "/", &save_ptr)), i++)
    {
      //Is next directory exist?
      if(struct_dir!= NULL &&
        dir_lookup_dir(struct_dir, token, &inode))
      {
        dir_close(struct_dir);
        struct_dir = dir_open(inode);
      }
      // Next directory doesn't exist
      else
      {
        dir_close(struct_dir);
        inode_close(inode);
        free(str);
        return false;
      }
    }



    //Now, dir is the destination directory
    if(struct_dir == NULL)
    {
      free(str);
      return false;
    }
    else
    {
      bool success;
      success = (dir_add_dir_with_allocate(struct_dir, token));
      free(str);

      dir_close(struct_dir);
      return success;
    }

  }
}

//Check whether file is directory or not
bool isdir (int fd)
{
  struct file * f;
  f = get_file(fd);
  return (f != NULL) ? filesys_is_directory(f) : false;
}

//Get sector number of file
int inumber (int fd)
{
  struct file * f;
  f = get_file(fd);
  return (f != NULL) ? inode_get_inumber(file_get_inode(f)) : false;
}

//Read Directory entry
bool readdir (int fd, char name[READDIR_MAX_LEN + 1])
{
  struct file * f;

  //Check Validity of fd
  if(!isdir(fd))
    return false;

  f = get_file(fd);

  return (f != NULL) ? dir_read_dir(f, name) : false;
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
