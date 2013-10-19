#include "fs.h"
#include "ext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO:    FIX: check if no files */
/*          ?: does the dir need to be made in sfs_open() if the */
/*             dirname is not found */

/* constant of how many bits in one freemap entry */
#define SFS_NBITS_IN_FREEMAP_ENTRY (sizeof(u32)*8)

/* in-memory superblock (in consistent with the disk copy) */
static sfs_superblock_t sb;
/* freemap, u32 array (in consistent with the disk copy) */
static u32 *freemap;
/* file descriptor table */
static fd_struct_t fdtable[SFS_MAX_OPENED_FILES];

/* 
 * Flush the in-memory freemap to disk 
 */
static void sfs_flush_freemap()
{
	size_t i;
	blkid bid = 1;
	char *p = (char *)freemap;
	/* TODO: write freemap block one by one */
	sfs_write_block(freemap, 1);
    
}

/* 
 * Allocate a free block, mark it in the freemap and flush the freemap to disk
 */
static blkid sfs_alloc_block()
{
	u32 size = sb.nfreemap_blocks * BLOCK_SIZE / sizeof(u32);	
	u32 i, j;
	/* TODO: find a freemap entry that has a free block */
    for(i = 0; i < 32; i++){
        int temp = 0;
        for(j=0x1; j < 0x8000; j <<= 1){
            if(i == 0 && j == 0x1){
                temp = 3;
                j = 0x4;
            }
            if((freemap[i] & j) == 0){
                freemap[i] = freemap[i] | j;
                sfs_flush_freemap();
                return i*32 + temp;
            }
            else{
                temp++;
            }
        }
    }
	/* TODO: find out which bit in the entry is zero,
	   set the bit, flush and return the bid
	*/
	return 0;
}

/*
 * Free a block, unmark it in the freemap and flush
 */
static void sfs_free_block(blkid bid)
{
	/* TODO find the entry and bit that correspond to the block */
	int entry_loc;
	int bit_loc;
    entry_loc = bid/32;
    bit_loc = bid%32;
    u32 bitMask = 0x1;
    bitMask <<= bit_loc;
	/* TODO unset the bit and flush the freemap */
    freemap[entry_loc] = freemap[entry_loc] ^ bitMask;
    sfs_flush_freemap();
}

/* 
 * Resize a file.
 * This file should be opened (in the file descriptor table). The new size
 * should be larger than the old one (not supposed to shrink a file)
 */
static void sfs_resize_file(int fd, u32 new_size)
{
	/* the length of content that can be hold by a full frame (in bytes) */
	int frame_size = BLOCK_SIZE * SFS_FRAME_COUNT;
	/* old file size */
	int old_size = fdtable[fd].inode.size;
	/* how many frames are used before resizing */
	int old_nframe = (old_size + frame_size -1) / frame_size;
	/* how many frames are required after resizing */
	int new_nframe = (new_size + frame_size - 1) / frame_size;
	int i, j;
	blkid frame_bid = 0;
	sfs_inode_frame_t frame;

	/* TODO: check if new frames are required */
	
	/* TODO: allocate a full frame */

	/* TODO: add the new frame to the inode frame list
	   Note that if the inode is changed, you need to write it to the disk
	*/
}

/*
 * Get the bids of content blocks that hold the file content starting from cur
 * to cur+length. These bids are stored in the given array.
 * The caller of this function is supposed to allocate the memory for this
 * array. It is guaranteed that cur+length<size
 * 
 * This function returns the number of bids being stored to the array.
 */
static u32 sfs_get_file_content(blkid *bids, int fd, u32 cur, u32 length)
{
	/* the starting block of the content */
	u32 start;
	/* the ending block of the content */
	u32 end;
	u32 i;
	sfs_inode_frame_t frame;

	/* TODO: find blocks between start and end.
	   Transverse the frame list if needed
	*/
	return 0;
}

/*
 * Find the directory of the given name.
 *
 * Return block id for the directory or zero if not found
 */
static blkid sfs_find_dir(char *dirname)
{
	blkid dir_bid = 0;
	sfs_dirblock_t dir;
	/* TODO: start from the sb.first_dir, treverse the linked list */
    if(sb.first_dir != 0){
        dir_bid = sb.first_dir;
        sfs_read_block(&dir, dir_bid);
        if(strcmp(dirname, dir.dir_name) == 0)
            return dir_bid;
        while(dir.next_dir != 0){
            dir_bid = dir.next_dir;
            sfs_read_block(&dir, dir_bid);
            if(strcmp(dirname, dir.dir_name) == 0)
                return dir_bid;
        }
    }
	return 0;
}

/*
 * Create a SFS with one superblock, one freemap block and 1022 data blocks
 *
 * The freemap is initialized be 0x3(11b), meaning that
 * the first two blocks are used (sb and the freemap block).
 *
 * This function always returns zero on success.
 */
int sfs_mkfs()
{
	/* one block in-memory space for freemap (avoid malloc) */
	static char freemap_space[BLOCK_SIZE];
	int i;
	sb.magic = SFS_MAGIC;
	sb.nblocks = 1024;
	sb.nfreemap_blocks = 1;
	sb.first_dir = 0;
	for (i = 0; i < SFS_MAX_OPENED_FILES; ++i) {
		/* no opened files */
		fdtable[i].valid = 0;
	}
	sfs_write_block(&sb, 0);
	freemap = (u32 *)freemap_space;
	memset(freemap, 0, BLOCK_SIZE);
	/* just to enlarge the whole file */
	sfs_write_block(freemap, sb.nblocks);
	/* initializing freemap */
	freemap[0] = 0x3; /* 11b, freemap block and sb used*/
	sfs_write_block(freemap, 1);
	memset(&sb, 0, BLOCK_SIZE);
	return 0;
}

/*
 * Load the super block from disk and print the parameters inside
 */
sfs_superblock_t *sfs_print_info()
{
	/* TODO: load the superblock from disk and print*/
    sfs_read_block(&sb, 0);
    printf("Magic: %d, nblocks: %d, nfreemap_blocks: %d, first_dir: %d\n", sb.magic, sb.nblocks, sb.nfreemap_blocks, sb.first_dir);
	return &sb;
}

/*
 * Create a new directory and return 0 on success.
 * If the dir already exists, return -1.
 */
int sfs_mkdir(char *dirname)
{
	/* TODO: test if the dir exists */
    blkid dir = sfs_find_dir(dirname);
    if(dir != 0)
        return -1;
	/* TODO: insert a new dir to the linked list */
    sfs_dirblock_t dirWrite, temp;
    blkid bid = sfs_alloc_block();
    strcpy(dirWrite.dir_name, dirname);
    dirWrite.next_dir = 0;
    int i;
    for(i = 0; i < SFS_DB_NINODES; i++){
        dirWrite.inodes[i] = 0;
    }
    sfs_write_block(&dirWrite, bid);
	/* TODO: start from the sb.first_dir, treverse the linked list */
    if(sb.first_dir != 0){
        sfs_read_block(&temp, sb.first_dir);
        blkid prevBID = sb.first_dir;
        while(temp.next_dir != 0){
            prevBID = temp.next_dir;
            sfs_read_block(&temp, prevBID);
        }
        
        temp.next_dir = bid;
        sfs_write_block(&temp, prevBID);
    }
    else{
        sb.first_dir = bid;
    }
    
    
	return 0;
}

/*
 * Remove an existing empty directory and return 0 on success.
 * If the dir does not exist or still contains files, return -1.
 */
int sfs_rmdir(char *dirname)
{
	/* TODO: check if the dir exists */
    blkid dir = sfs_find_dir(dirname);
    if(dir == 0)
        return -1;
	/* TODO: check if no files */
    sfs_dirblock_t dirRead, temp;
    sfs_read_block(&dirRead, dir);
    int i;
    for(i = 0; i < SFS_DB_NINODES; i++){
        if(dirRead.inodes[i] == 0){
            return -1;
        }
    }
	/* TODO: go thru the linked list and delete the dir*/
    sfs_read_block(&temp, sb.first_dir);
    if(sb.first_dir == dir){
        sb.first_dir = temp.next_dir;
        sfs_free_block(dir);
        return 0;
    }
    blkid prevBID = sb.first_dir;
    while(temp.next_dir != dir){
        prevBID = temp.next_dir;
        sfs_read_block(&temp, temp.next_dir);
    }
    temp.next_dir = dirRead.next_dir;
    sfs_write_block(&temp, prevBID);
    sfs_free_block(dir);
	return 0;
}

/*
 * Print all directories. Return the number of directories.
 */
int sfs_lsdir()
{
	/* TODO: go thru the linked list */
    if(sb.first_dir == 0){
        return 0;
    }
    int i = 1;
    sfs_dirblock_t dir;
    sfs_read_block(&dir, sb.first_dir);
    while(dir.next_dir != 0){
        int ii;
//        printf("IN LOOP");
//        for(ii = 0; ii < sizeof(dir.dir_name); ii++){
//            printf("%c", dir.dir_name[ii]);
//        }
//        printf("AA\n");
        i++;
        sfs_read_block(&dir, dir.next_dir);
    }
    int ii;
//    for(ii = 0; ii < sizeof(dir.dir_name); ii++){
//        printf("%c", dir.dir_name[ii]);
//    }
//    printf("HERE: %d\n", i);
	return i;
}

/*
 * Open a file. If it does not exist, create a new one.
 * Allocate a file desriptor for the opened file and return the fd.
 */
int sfs_open(char *dirname, char *name)
{
	blkid dir_bid = 0, inode_bid = 0;
	sfs_inode_t *inode;
	sfs_dirblock_t dir;
	int fd;
	int i;

	/* TODO: find a free fd number */
    for(i = 0; i < SFS_MAX_OPENED_FILES; i++){
        if(fdtable[i].valid == 0){
            fd = i;
            break;
        }
    }
	/* TODO: find the dir first */
    dir_bid = sfs_find_dir(dirname);
    if(dir_bid == 0)
        return -1;
    sfs_read_block(&dir, dir_bid);

	/* TODO: traverse the inodes to see if the file exists.
	   If it exists, load its inode. Otherwise, create a new file.
	*/
    int free = -1;
    for(i = 0; i < SFS_DB_NINODES; i++){
        printf("\t%d\n", i);
        inode_bid = dir.inodes[i];
        printf("\t%d\n", inode_bid);
        if(inode_bid > 2){
            printf("HERE   k\n");
            sfs_read_block(inode, inode_bid);
            printf("\t\t%d\n", inode_bid);
            if(strcmp(name, (*inode).file_name)){
                fdtable[fd].inode = *inode;
                fdtable[fd].inode_bid = inode_bid;
                fdtable[fd].dir_bid = dir_bid;
                fdtable[fd].cur = 0;
                fdtable[fd].valid = 1;
                return fd;
            }
        }
        
        if(free == -1){
            free = i;
            printf("HERE   k\n");
        }
    }
    printf("HERE\n");
    if(free == -1)
        return -1;
    
	/* TODO: create a new file */
    inode_bid = sfs_alloc_block();
    (*inode).size = 0;
    (*inode).first_frame = -1;
    strcpy((*inode).file_name, name);
    sfs_write_block(inode, inode_bid);
    dir.inodes[free] = inode_bid;
    sfs_write_block(&dir, dir_bid);
	return fd;
}

/*
 * Close a file. Just mark the valid field to be zero.
 */
int sfs_close(int fd)
{
	/* TODO: mark the valid field */
	return 0;
}

/*
 * Remove/delete an existing file
 *
 * This function returns zero on success.
 */
int sfs_remove(int fd)
{
	blkid frame_bid;
	sfs_dirblock_t dir;
	int i;

	/* TODO: update dir */

	/* TODO: free inode and all its frames */

	/* TODO: close the file */
	return 0;
}

/*
 * List all the files in all directories. Return the number of files.
 */
int sfs_ls()
{
	/* TODO: nested loop: traverse all dirs and all containing files*/
    int dir_bid = sb.first_dir;
    if(dir_bid == 0)
        return 0;
    sfs_dirblock_t dir;
    int files = 0;
    sfs_read_block(&dir, dir_bid);
    do {
        printf("%s\n", dir.dir_name);
        int i;
        for(i = 0; i < SFS_DB_NINODES; i++){
            sfs_inode_t inode;
            blkid inode_bid;
            if(inode_bid = dir.inodes[i] > 2){
                sfs_read_block(&inode, inode_bid);
                printf("\t%s\n", inode.file_name);
                files++;
            }
        }
        
    } while (dir.next_dir != 0);
	return files;
}

/*
 * Write to a file. This function can potentially enlarge the file if the 
 * cur+length exceeds the size of file. Also you should be aware that the
 * cur may already be larger than the size (due to sfs_seek). In such
 * case, you will need to expand the file as well.
 * 
 * This function returns number of bytes written.
 */
int sfs_write(int fd, void *buf, int length)
{
	int remaining, offset, to_copy;
	blkid *bids;
	int i, n;
	char *p = (char *)buf;
	char tmp[BLOCK_SIZE];
	u32 cur = fdtable[fd].cur;

	/* TODO: check if we need to resize */
	
	/* TODO: get the block ids of all contents (using sfs_get_file_content() */

	/* TODO: main loop, go through every block, copy the necessary parts
	   to the buffer, consult the hint in the document. Do not forget to 
	   flush to the disk.
	*/
	/* TODO: update the cursor and free the temp buffer
	   for sfs_get_file_content()
	*/
	return 0;
}

/*
 * Read from an opend file. 
 * Read can not enlarge file. So you should not read outside the size of 
 * the file. If the read exceeds the file size, its result will be truncated.
 *
 * This function returns the number of bytes read.
 */
int sfs_read(int fd, void *buf, int length)
{
	int remaining, to_copy, offset;
	blkid *bids;
	int i, n;
	char *p = (char *)buf;
	char tmp[BLOCK_SIZE];
	u32 cur = fdtable[fd].cur;

	/* TODO: check if we need to truncate */
	/* TODO: similar to the sfs_write() */
	return 0;
}

/* 
 * Seek inside the file.
 * Loc is the starting point of the seek, which can be:
 * - SFS_SEEK_SET represents the beginning of the file.
 * - SFS_SEEK_CUR represents the current cursor.
 * - SFS_SEEK_END represents the end of the file.
 * Relative tells whether to seek forwards (positive) or backwards (negative).
 * 
 * This function returns 0 on success.
 */
int sfs_seek(int fd, int relative, int loc)
{
	/* TODO: get the old cursor, change it as specified by the parameters */
	return 0;
}

/*
 * Check if we reach the EOF(end-of-file).
 * 
 * This function returns 1 if it is EOF, otherwise 0.
 */
int sfs_eof(int fd)
{
	/* TODO: check if the cursor has gone out of bound */
	return 0;
}
