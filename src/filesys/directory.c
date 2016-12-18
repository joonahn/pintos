#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"

/* A directory. */
struct dir 
  {
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
  };

/* A single directory entry. */
struct dir_entry 
  {
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    bool is_dir;
    bool in_use;                        /* In use or free? */
  };

/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt)
{
  bool success;
  struct inode *inode;
  success = inode_create_dir (sector, entry_cnt * sizeof (struct dir_entry));
  inode = inode_open(sector);
  inode_mark_dir(inode);
  inode_close(inode);
  return success;
}

/* Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
struct dir *
dir_open (struct inode *inode) 
{
  struct dir *dir = calloc (1, sizeof *dir);
  if (inode != NULL && dir != NULL)
    {
      dir->inode = inode;
      dir->pos = 0;
      return dir;
    }
  else
    {
      inode_close (inode);
      free (dir);
      return NULL; 
    }
}

/* Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
struct dir *
dir_open_root (void)
{
  return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir) 
{
  return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir) 
{
  if (dir != NULL)
    {
      inode_close (dir->inode);
      free (dir);
    }
}

/* Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir) 
{
  return dir->inode;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp) 
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  ofs = 0;

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
  {
    // printf("e.in_use: %d, e.name: %s\n", e.in_use, e.name );
    if (e.in_use && !strcmp (name, e.name)) 
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
    }
  return false;
}

/* Searches DIR for a *FILE* with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup_file (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp) 
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  ofs = 0;

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
  {
    // printf("e.in_use: %d, e.name: %s\n", e.in_use, e.name );
    if (e.in_use && (e.is_dir == false) && !strcmp (name, e.name)) 
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
    }
  return false;
}

/* Searches DIR for a DIR with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup_dir (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp) 
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  ofs = 0;

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
  {
    // printf("e.in_use: %d, e.name: %s\n", e.in_use, e.name );
    if (e.in_use && e.is_dir && !strcmp (name, e.name)) 
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
    }
  return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
            struct inode **inode) 
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (lookup (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
  {
    *inode = NULL;
  }

  return *inode != NULL;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup_file (const struct dir *dir, const char *name,
            struct inode **inode) 
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (lookup_file (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
  {
    *inode = NULL;
  }

  return *inode != NULL;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup_dir (const struct dir *dir, const char *name,
            struct inode **inode) 
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (lookup_dir (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
  {
    *inode = NULL;
  }

  return *inode != NULL;
}

/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add (struct dir *dir, const char *name, block_sector_t inode_sector)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    goto done;

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.
     
     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  e.is_dir = false;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

 done:
  return success;
}

/* Adds a folder named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add_dir (struct dir *dir, const char *name, block_sector_t inode_sector)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    goto done;

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.
     
     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  e.is_dir = true;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

 done:
  return success;
}

/* Adds a folder named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add_dir_with_allocate (struct dir *dir, const char *name)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;
  block_sector_t inode_sector = 0;
  struct dir * new_dir;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    goto done;

  /* write inode to disk */
  success =     (dir != NULL
                  && free_map_allocate (1, &inode_sector));
  if (!success && inode_sector != 0) 
  {
    free_map_release (inode_sector, 1);
    goto done;
  }

  dir_create(inode_sector, 16);
  new_dir = dir_open(inode_open(inode_sector));
  dir_close(new_dir);

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.
     
     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  e.is_dir = true;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

 done:
  return success;
}

/* Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *dir, const char *name) 
{
  struct dir_entry e;
  struct inode *inode = NULL;
  bool success = false;
  off_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Find directory entry. */
  if (!lookup (dir, name, &e, &ofs))
    goto done;

  /* Open inode. */
  inode = inode_open (e.inode_sector);
  if (inode == NULL)
    goto done;

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e) 
    goto done;

  /* Remove inode. */
  inode_remove (inode);
  success = true;

 done:
  inode_close (inode);
  return success;
}

/* Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{
  struct dir_entry e;

  while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e) 
    {
      dir->pos += sizeof e;
      if (e.in_use)
        {
          strlcpy (name, e.name, NAME_MAX + 1);
          return true;
        } 
    }
  return false;
}

struct dir *dir_open_abs(const char *abs_name, char * filename_buffer, bool * is_dir)
{
  block_sector_t inode_sector = 0;
  int depth = -1, i, length;
  struct dir *dir = NULL;
  struct inode *inode = NULL;
  char * str, *token, *save_ptr;
  bool _is_dir = false;
  bool success;

  //Check path
  length = strlen(abs_name);
  if(abs_name[0]!= '/')
    return NULL;
  _is_dir = (abs_name[length-1] == '/');

  //Parse string
  for (i = 0; i < length; ++i)
  {
    if(abs_name[i] == '/')
    {
      if(i != (length-1) && abs_name[i+1] == '/')
        return NULL;
      depth ++;
    }
  }
  if(_is_dir)
    depth --;

  //Open Folders
  str = malloc(length + 1);
  memcpy(str, abs_name, length + 1);
  dir = dir_open_root ();
  token = strtok_r(str, "/", &save_ptr);
  for (i = 0; i < (depth); ++i)
  {
    //Is next directory exist?
    if(dir!= NULL &&
      dir_lookup_dir(dir, token, &inode))
    {
      dir_close(dir);
      dir = dir_open(inode);
    }
    // Next directory doesn't exist
    else
    {
      dir_close(dir);
      free(str);
      return NULL;
    }
    token = strtok_r(NULL, "/", &save_ptr);
  }
  
  //Now, dir is the destination directory
  if(is_dir != NULL)
    (*is_dir) = _is_dir;
  if(filename_buffer != NULL && strlen(token)<=NAME_MAX)
    memcpy(filename_buffer, token, strlen(token) + 1);

  free(str);

  return dir;

}

/* open absolute directory string end with '/' */
struct dir *dir_open_abs_dir(const char *abs_name)
{
  //relative path
  int length;
  struct dir * struct_dir = NULL;
  struct inode *inode = NULL;
  char *str, *token, *save_ptr;

  //Open Folders
  length = strlen(abs_name);
  str = malloc(length + 1);
  memcpy(str, abs_name, length + 1);
  struct_dir = dir_open_root ();


  for (token = strtok_r (str, "/", &save_ptr); token != NULL;
       token = strtok_r (NULL, "/", &save_ptr))
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
      free(str);
      return NULL;
    }
  }

  return struct_dir;
}

//check dir is empty
bool dir_is_empty(struct dir *dir)
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dir != NULL);

  ofs = 0;

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
  {
    if (e.in_use) 
      {
        return false;
      }
    }
  return true;

}

bool dir_is_same(const char *str1, const char *str2)
{
  char * buffer;
  int nstr1, nstr2;
  if(strcmp(str1, str2)==0)
    return true;
  nstr1 = strlen(str1);
  nstr2 = strlen(str2);
  if(nstr1 == nstr2)
    return false;
  if(nstr1 == (nstr2 + 1))
  {
    buffer = malloc(nstr1 + 1);
    memcpy(buffer, str2, nstr2);
    buffer[nstr2] = '/';
    buffer[nstr2+1] = 0;
    if(strcmp(str1, buffer)==0)
      return true;
  }

  if(nstr2 == (nstr1 + 1))
  {
    buffer = malloc(nstr2 + 1);
    memcpy(buffer, str1, nstr1);
    buffer[nstr1] = '/';
    buffer[nstr1+1] = 0;
    if(strcmp(str2, buffer)==0)
      return true;
  }
  return false;
}

bool dir_read_dir(struct file *f, char * str)
{
  struct dir_entry e;
  size_t ofs;
  off_t bytes_read;

  for (; file_read (f, &e, sizeof e) == sizeof e;) 
  {
    // printf("e.in_use: %d, e.name: %s\n", e.in_use, e.name );
    if (e.in_use) 
    {
      memcpy(str, e.name, strlen(e.name) + 1);
      return true;
    }
  }
  return false;
}
