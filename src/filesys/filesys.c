#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  block_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root ();
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

  return success;
}


/* create file with absolute path,
   must be started with '/'
   file: must not end with '/' 
   directory : must end with '/' */
bool filesys_abs_create (const char *abs_name, off_t initial_size)
{
  block_sector_t inode_sector = 0;
  struct dir *dir = NULL;
  struct inode *inode = NULL;
  bool is_dir = false;
  char filename[NAME_MAX + 1];
  bool success;

  dir = dir_open_abs(abs_name, filename, &is_dir);
  
  //Now, dir is the destination directory
  if(dir==NULL)
  {
    dir_close(dir);
    return false;
  }

  // printf("name : %s, is_dir %d\n", abs_name, is_dir);
  // printf("filename : %s, is_dir %d\n", filename, is_dir);
  // printf("filename length : %d, is_dir %d\n", strlen(filename), is_dir);
  if(is_dir)
  {
    success = (dir != NULL
                && free_map_allocate (1, &inode_sector)
                && inode_create (inode_sector, initial_size)
                && dir_add_dir (dir, filename, inode_sector));
  }
  else
  {
    success = (dir != NULL
                && free_map_allocate (1, &inode_sector)
                && inode_create (inode_sector, initial_size)
                && dir_add (dir, filename, inode_sector));
  }
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

struct file *filesys_abs_open (const char *abs_name)
{
  struct dir *dir = NULL;
  struct inode *inode = NULL;
  char filename[NAME_MAX + 1];
  bool is_dir = false;

  //root open
  if(strcmp("/", abs_name) == 0)
    return file_open(inode_open(ROOT_DIR_SECTOR));

  dir = dir_open_abs(abs_name, filename, &is_dir);
  
  //Now, dir is the destination directory
  if(dir==NULL)
  {
    dir_close(dir);
    return NULL;
  }

  if(is_dir)
    dir_lookup_dir(dir, filename, &inode);
  else
    dir_lookup(dir, filename, &inode);

  dir_close (dir);
  return file_open(inode);
}

// !!!! If you delete a dir file, you MUST CHECK whether it is EMPTY
bool filesys_abs_remove (const char *abs_name)
{
  block_sector_t inode_sector = 0;
  struct dir *dir, *child_dir;
  struct inode *inode = NULL;
  bool is_dir = false;
  char filename[NAME_MAX + 1];
  bool success;

  dir = dir_open_abs(abs_name, filename, NULL);
  
  //Now, dir is the destination directory
  if(dir==NULL)
  {
    dir_close(dir);
    return false;
  }

  //Check whether dir is empty
  if(dir_lookup_dir(dir, filename, &inode))   
  {
    is_dir = true;
    child_dir = dir_open(inode);    
    if(!dir_is_empty(child_dir))
    {
      dir_close(child_dir);
      dir_close(dir);
      return false;
    }
    dir_close(child_dir);
  }

  //Check whether the file is opened
  if(is_dir)
  {
    dir_lookup(dir, filename, &inode);
    if(inode != NULL)
    {
      if(inode_open_cnt(inode) > 1)
      {
          // printf("while removing %s\n", abs_name);
          // printf("inode count : %d\n", inode_open_cnt(inode));
          dir_close(dir);
          inode_close(inode);
          return false;
      }
    }
  }

  success = dir != NULL && dir_remove (dir, filename);

  // if (!success && inode_sector != 0) 
  //   free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

void filesys_change_rel_to_abs(const char *rel_name, const char *abs_cur_path, char ** abs_name)
{
  int length, i;
  char * str, *tmp, *curpath, *save_ptr, *token;

  // Initialize
  length = strlen(rel_name);
  str = malloc(length + 1);
  memcpy(str, rel_name, length + 1);
  length = strlen(abs_cur_path);
  curpath = malloc(length + 1);
  memcpy(curpath, abs_cur_path, length + 1);

  for (token = strtok_r (str, "/", &save_ptr); token != NULL;
       token = strtok_r (NULL, "/", &save_ptr))
  {
    if(strcmp(".", token) == 0)
      continue;

    if(strcmp("..", token) == 0)
    {
      if(strcmp(curpath, "/") == 0)
        continue;
      for(i = strlen(curpath)-1; i >0; i--)
      {
        if(curpath[i-1] == '/')
        {
          curpath[i] = 0;
          break;
        }
      }
      continue;
    }

    tmp = (char *)malloc(strlen(curpath) + strlen(token) + 2);
    memcpy(tmp, curpath, strlen(curpath));
    memcpy(tmp + strlen(curpath), token, strlen(token));
    memcpy(tmp + strlen(curpath) + strlen(token), "/", 2);
    free(curpath);
    curpath = tmp;
  }
  // remove trailing '/' if rel_name has no '/'
  if(rel_name[strlen(rel_name) - 1] != '/')
  {
    if(strcmp("/", curpath)!=0)
      curpath[strlen(curpath) - 1] = 0;
  }
  *abs_name = curpath;
}

bool filesys_is_directory(struct file *f)
{
  return inode_is_dir(file_get_inode (f));
}


/* Formats the file system. */
static void
do_format (void)
{
  struct dir * dir;
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");

  dir = dir_open_root();
  dir_close(dir);

  free_map_close ();
  printf ("done.\n");
}


static int get_depth(const char * abs_path)
{
  int depth = -1;
  char * save_ptr, *token;
  char * str = (char *)malloc(strlen(abs_path) + 1);

  memcpy(str, abs_path, strlen(abs_path) + 1);

  //Calculate depth
  for (token = strtok_r (str, "/", &save_ptr); token != NULL;
       token = strtok_r (NULL, "/", &save_ptr))
  {
    depth ++;
  }

  return depth;

}

void filesys_trim_string(char * str)
{
  char * buffer;
  int start, finish, length;
  length = strlen(str);
  if(length < 1)
    return;


  buffer = malloc(length + 1);
  memcpy(buffer, str, length + 1);

  for(start = 0; start < length ; start ++)
  {
    if(str[start] != ' ' && str[start] != '\t')
      break;
  }
  for(finish = (length - 1); finish>=0;finish--)
  {
    if(str[finish] != ' ' && str[finish] != '\t')
      break;
  }
  if(start>finish)
  {
    str[0] = 0;
    free(buffer);
    return;
  }
  memcpy(str, buffer + start, finish-start + 1);
  str[finish - start + 1] = 0;
  free(buffer);
  return;
}