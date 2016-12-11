#include "filesys/file.h"
#include <debug.h>
#include "filesys/inode.h"
#include "threads/malloc.h"

/* An open file. */
struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

/* Opens a file for the given INODE, of which it takes ownership,
   and returns the new file.  Returns a null pointer if an
   allocation fails or if INODE is null. */
struct file *
file_open (struct inode *inode) 
{
  struct file *file = calloc (1, sizeof *file);
  if (inode != NULL && file != NULL)
    {
      file->inode = inode;
      file->pos = 0;
      file->deny_write = false;
      return file;
    }
  else
    {
      inode_close (inode);
      free (file);
      return NULL; 
    }
}

/* Opens and returns a new file for the same inode as FILE.
   Returns a null pointer if unsuccessful. */
struct file *
file_reopen (struct file *file) 
{
  return file_open (inode_reopen (file->inode));
}

/* Closes FILE. */
void
file_close (struct file *file) 
{
  if (file != NULL)
    {
      file_allow_write (file);
      inode_close (file->inode);
      free (file); 
    }
}

/* Returns the inode encapsulated by FILE. */
struct inode *
file_get_inode (struct file *file) 
{
  return file->inode;
}

/* Reads SIZE bytes from FILE into BUFFER,
   starting at the file's current position.
   Returns the number of bytes actually read,
   which may be less than SIZE if end of file is reached.
   Advances FILE's position by the number of bytes read. */
off_t
file_read (struct file *file, void *buffer, off_t size) 
{
  off_t bytes_read = inode_read_at (file->inode, buffer, size, file->pos);
  file->pos += bytes_read;
  return bytes_read;

  // off_t remain_size = size;
  // off_t bytes_read = 0;
  // off_t offset = file->pos;
  // uint8_t *bounce = NULL;
  // // char buffer[512];

  // while(remain_size > 0)
  // {
  //   block_sector_t sector_idx = byte_to_sector (file->inode, offset);
  //   int sector_ofs = offset % BLOCK_SECTOR_SIZE;

  //   /* Bytes left in inode, bytes left in sector, lesser of the two. */
  //   off_t inode_left = inode_length (file->inode) - offset;
  //   int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
  //   int min_left = inode_left < sector_left ? inode_left : sector_left;

  //   /* Number of bytes to actually copy out of this sector. */
  //   int chunk_size = remain_size < min_left ? remain_size : min_left;
  //   if (chunk_size <= 0)
  //     break;

  //   if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
  //   {
  //     /* Read full sector directly into caller's buffer. */
  //     cache_block_read (sector_idx, buffer + bytes_read);
  //   }
  //   else 
  //   {
  //     /* Read sector into bounce buffer, then partially copy
  //        into caller's buffer. */
  //     if (bounce == NULL) 
  //       {
  //         bounce = malloc (BLOCK_SECTOR_SIZE);
  //         if (bounce == NULL)
  //           break;
  //       }
  //     cache_block_read (sector_idx, bounce);
  //     memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
  //   }

  //   remain_size -= chunk_size;
  //   offset += chunk_size;
  //   bytes_read += chunk_size;
  // }
  // free (bounce);

  // file->pos += bytes_read;
  // return bytes_read;
}

/* Reads SIZE bytes from FILE into BUFFER,
   starting at offset FILE_OFS in the file.
   Returns the number of bytes actually read,
   which may be less than SIZE if end of file is reached.
   The file's current position is unaffected. */
off_t
file_read_at (struct file *file, void *buffer, off_t size, off_t file_ofs) 
{
  return inode_read_at (file->inode, buffer, size, file_ofs);
}

/* Writes SIZE bytes from BUFFER into FILE,
   starting at the file's current position.
   Returns the number of bytes actually written,
   which may be less than SIZE if end of file is reached.
   (Normally we'd grow the file in that case, but file growth is
   not yet implemented.)
   Advances FILE's position by the number of bytes read. */
off_t
file_write (struct file *file, const void *buffer, off_t size) 
{
  off_t bytes_written = inode_write_at (file->inode, buffer, size, file->pos);
  file->pos += bytes_written;
  return bytes_written;

  // off_t remain_size = size;
  // off_t bytes_written = 0;
  // off_t offset = file->pos;
  // uint8_t *bounce = NULL;
  // // char buffer[512];

  // while(remain_size > 0)
  // {
  //   block_sector_t sector_idx = byte_to_sector (file->inode, offset);
  //   int sector_ofs = offset % BLOCK_SECTOR_SIZE;

  //   /* Bytes left in inode, bytes left in sector, lesser of the two. */
  //   off_t inode_left = inode_length (file->inode) - offset;
  //   int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
  //   int min_left = inode_left < sector_left ? inode_left : sector_left;

  //   /* Number of bytes to actually copy out of this sector. */
  //   int chunk_size = remain_size < min_left ? remain_size : min_left;
  //   if (chunk_size <= 0)
  //     break;

  //   if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
  //   {
  //     /* Read full sector directly into caller's buffer. */
  //     cache_block_write (sector_idx, buffer + bytes_written);
  //   }
  //   else 
  //   {
  //     /* Read sector into bounce buffer, then partially copy
  //        into caller's buffer. */
  //     if (bounce == NULL) 
  //       {
  //         bounce = malloc (BLOCK_SECTOR_SIZE);
  //         if (bounce == NULL)
  //           break;
  //       }

  //     /* If the sector contains data before or after the chunk
  //        we're writing, then we need to read in the sector
  //        first.  Otherwise we start with a sector of all zeros. */
  //     if (sector_ofs > 0 || chunk_size < sector_left) 
  //       cache_block_read (sector_idx, bounce);
  //     else
  //       memset (bounce, 0, BLOCK_SECTOR_SIZE);
  //     memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
  //     cache_block_write (sector_idx, bounce);
  //   }

  //   remain_size -= chunk_size;
  //   offset += chunk_size;
  //   bytes_written += chunk_size;
  // }
  // free (bounce);

  // file->pos += bytes_written;
  // return bytes_written;

}

/* Writes SIZE bytes from BUFFER into FILE,
   starting at offset FILE_OFS in the file.
   Returns the number of bytes actually written,
   which may be less than SIZE if end of file is reached.
   (Normally we'd grow the file in that case, but file growth is
   not yet implemented.)
   The file's current position is unaffected. */
off_t
file_write_at (struct file *file, const void *buffer, off_t size,
               off_t file_ofs) 
{
  return inode_write_at (file->inode, buffer, size, file_ofs);
  
  // off_t remain_size = size;
  // off_t bytes_written = 0;
  // off_t offset = file_ofs;
  // uint8_t *bounce = NULL;
  // // char buffer[512];

  // while(remain_size > 0)
  // {
  //   block_sector_t sector_idx = byte_to_sector (file->inode, offset);
  //   int sector_ofs = offset % BLOCK_SECTOR_SIZE;

  //   /* Bytes left in inode, bytes left in sector, lesser of the two. */
  //   off_t inode_left = inode_length (file->inode) - offset;
  //   int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
  //   int min_left = inode_left < sector_left ? inode_left : sector_left;

  //   /* Number of bytes to actually copy out of this sector. */
  //   int chunk_size = remain_size < min_left ? remain_size : min_left;
  //   if (chunk_size <= 0)
  //     break;

  //   if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
  //   {
  //     /* Read full sector directly into caller's buffer. */
  //     cache_block_write (sector_idx, buffer + bytes_written);
  //   }
  //   else 
  //   {
  //     /* Read sector into bounce buffer, then partially copy
  //        into caller's buffer. */
  //     if (bounce == NULL) 
  //       {
  //         bounce = malloc (BLOCK_SECTOR_SIZE);
  //         if (bounce == NULL)
  //           break;
  //       }

  //     /* If the sector contains data before or after the chunk
  //        we're writing, then we need to read in the sector
  //        first.  Otherwise we start with a sector of all zeros. */
  //     if (sector_ofs > 0 || chunk_size < sector_left) 
  //       cache_block_read (sector_idx, bounce);
  //     else
  //       memset (bounce, 0, BLOCK_SECTOR_SIZE);
  //     memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
  //     cache_block_write (sector_idx, bounce);
  //   }

  //   remain_size -= chunk_size;
  //   offset += chunk_size;
  //   bytes_written += chunk_size;
  // }
  // free (bounce);

  // // file->pos += bytes_written;
  // return bytes_written;
}

/* Prevents write operations on FILE's underlying inode
   until file_allow_write() is called or FILE is closed. */
void
file_deny_write (struct file *file) 
{
  ASSERT (file != NULL);
  if (!file->deny_write) 
    {
      file->deny_write = true;
      inode_deny_write (file->inode);
    }
}

/* Re-enables write operations on FILE's underlying inode.
   (Writes might still be denied by some other file that has the
   same inode open.) */
void
file_allow_write (struct file *file) 
{
  ASSERT (file != NULL);
  if (file->deny_write) 
    {
      file->deny_write = false;
      inode_allow_write (file->inode);
    }
}

/* Returns the size of FILE in bytes. */
off_t
file_length (struct file *file) 
{
  ASSERT (file != NULL);
  return inode_length (file->inode);
}

/* Sets the current position in FILE to NEW_POS bytes from the
   start of the file. */
void
file_seek (struct file *file, off_t new_pos)
{
  ASSERT (file != NULL);
  ASSERT (new_pos >= 0);
  file->pos = new_pos;
}

/* Returns the current position in FILE as a byte offset from the
   start of the file. */
off_t
file_tell (struct file *file) 
{
  ASSERT (file != NULL);
  return file->pos;
}

bool
file_get_deny (struct file *file)
{
  ASSERT (file !=NULL);
  return file->deny_write;
}
