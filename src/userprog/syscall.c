#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"

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
  printf ("system call!\n");
  printf ("%s: exit(%d)\n", "args-none", 0);
  printf("syscall number : %d\n", syscall_number);
  switch(syscall_number)
  {
  	case SYS_WRITE:
  	{
  	  int fd;
  	  char *buffer;
  	  unsigned length;

  	  //extract data from stack
  	  hex_dump((((int)f->esp)), /*(char *)pagedir_get_page(cur->pagedir, (((int)f->esp)))*/f->esp, 64, true);

  	  fd = *((int *)pagedir_get_page(cur->pagedir, (f->esp) + 4));
  	  buffer = (char *)pagedir_get_page(cur->pagedir, (f->esp) + 8);
  	  //buffer = (f->esp) + 8;
  	  length = *((unsigned *)pagedir_get_page(cur->pagedir, (f->esp) + 12));

  	  printf("write syscall!, fd:%d, buffer:%s, length:%d\n", fd, buffer, length);

  	  break;
  	}
  	case SYS_EXIT:
  	{
  	  printf("exit syscall!");
	  process_exit();
      thread_exit ();
  	}
  }
  //process_exit();
  //thread_exit ();
}

void
exit (int status)
{
  process_exit();

}