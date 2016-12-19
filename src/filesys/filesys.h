#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

/* Block device that contains the file system. */
struct block *fs_device;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);

// filesys function for absolute path
bool filesys_abs_create (const char *abs_name, off_t initial_size);
struct file *filesys_abs_open (const char *abs_name);
void filesys_change_rel_to_abs(const char *rel_name, const char *abs_cur_path, char ** abs_name);
void filesys_trim_string(char * str);
bool filesys_abs_remove (const char *abs_name);
bool filesys_is_directory(struct file *);

#endif /* filesys/filesys.h */
