#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <debug.h>
#include <stdbool.h>
#include <stdint.h>
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include <list.h>

typedef int mapid_t;
typedef int pid_t;

// mmap_id 
struct mmap_table
{
	int mapid;
	uint8_t * uaddr;
	uint32_t * pagedir;
	size_t size;
	struct file * file;

	//list info
	struct list_elem mmap_elem;
};

void syscall_init (void);
int arg_check(int, uint32_t *, uint32_t *, uint32_t *);

/* Projects 2 and later. */
void halt (void) NO_RETURN;
void exit (int status) NO_RETURN;
//pid_t exec (const char *file);
int wait (pid_t);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
mapid_t mmap(int fd, void *addr);
void munmap(mapid_t mapid);

#endif /* userprog/syscall.h */
