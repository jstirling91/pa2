#ifndef EXT_H_
#define EXT_H_

#include "fs.h"

void sfs_init_storage();
void sfs_close_storage();
int sfs_read_block(void *buf, blkid bid);
int sfs_write_block(void *buf, blkid bid);

#endif
