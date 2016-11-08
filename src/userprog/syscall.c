#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
 
  struct thread *cur = thread_current ();
  int syscall_number = *((int *)pagedir_get_page(cur->pagedir, f->esp));
  switch(syscall_number)
  {
  	case SYS_WRITE:
  	{
  	  int fd;
  	  char *buffer;
  	  unsigned length;

      //extract data from the stack
      fd = *((int*)pagedir_get_page(cur->pagedir, (f->esp) + 4));
      buffer =(char*)(*((int*)pagedir_get_page(cur->pagedir, (f->esp) + 8)));
      length = *((unsigned *)pagedir_get_page(cur->pagedir, (f->esp) + 12));
      if(fd==1)
      {
        putbuf(buffer, length);
      }
      else
        return;
  	  break;
  	}
  	case SYS_EXIT:
  	{
      int stat=*((int*)pagedir_get_page(cur->pagedir, (f->esp) + 4));
      cur->exitstat = stat;
//      hex_dump((((int)f->esp)),f->esp,64,true);
      printf ("%s: exit(%d)\n", cur->process_name, stat);
      thread_exit ();
  	}
  }
  //process_exit();
  //thread_exit ();
}
