#ifndef FILESYS_FDMAP_H
#define FILESYS_FDMAP_H

/* File Descriptor mapping information */
struct fdmap
{
  int fd;
  struct file * fp;
  //list information
  struct list_elem fdmap_elem;
};

#endif