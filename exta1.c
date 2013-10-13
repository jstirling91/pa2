#include "ext.h"
#undef BLOCK_SIZE
#include "a1/mydisk.h"

void report_latency(int latency) {}

int sfs_read_block(void *buf, blkid bid)
{
	mydisk_read_block(bid, buf);
	return 0;
}

int sfs_write_block(void *buf, blkid bid)
{
	mydisk_write_block(bid, buf);
	return 0;
}

void sfs_init_storage()
{
	mydisk_init("storage", 1024, 0); 
}

void sfs_close_storage()
{
	mydisk_close();
}

