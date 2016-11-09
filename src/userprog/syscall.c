#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  uint32_t *arg1, *arg2, *arg3;
  struct thread *cur = thread_current ();
  int syscall_number, *syscall_number_ptr;

  syscall_number_ptr = (int *)pagedir_get_page(cur->pagedir, f->esp);
  arg1 = (uint32_t *)pagedir_get_page(cur->pagedir, (f->esp) + 4);
  arg2 = (uint32_t *)pagedir_get_page(cur->pagedir, (f->esp) + 8);
  arg3 = (uint32_t *)pagedir_get_page(cur->pagedir, (f->esp) + 12);

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
      f->eax = process_execute(*((char **)arg1));
      break;
    }
    case SYS_WAIT:
    {
      f->eax = process_wait(*((int *)arg1));
      break;
    }
    case SYS_CLOSE:
    case SYS_TELL:
    case SYS_REMOVE:
    case SYS_OPEN:
    case SYS_FILESIZE:
      break;


    //#of arg : 2
    case SYS_SEEK:
    case SYS_CREATE:
      break;


    //#of arg : 3
    case SYS_READ:
      break;
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
  struct thread *cur = thread_current ();
  cur->exitstat = status;
  printf ("%s: exit(%d)\n", cur->process_name, stat);
  thread_exit ();
}

int write (int fd, const void *buffer, unsigned size)
{
  if(fd==1)
  {
    putbuf(buffer, length);
  }
  else
    //TODO: implement this
    return;
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