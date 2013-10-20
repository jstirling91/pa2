#include "fs.h"
#include "ext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO:    FIX: check if no files */
/*          ?: does the dir need to be made in sfs_open() if the */
/*             dirname is not found */
/*          BUG: infinit loop in open on test 7 */

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
    j = new_nframe - old_nframe;
    if(j == 0){
        return;
    }
	
	/* TODO: allocate a full frame */
    sfs_inode_t inode = fdtable[fd].inode;
    frame_bid = inode.first_frame;
    if(frame_bid == 0){
        frame_bid = sfs_alloc_block();
        //printf("FRAME_BID: %d\n", frame_bid);
        inode.first_frame = frame_bid;
        sfs_write_block(&inode, fdtable[fd].inode_bid);
        fdtable[fd].inode = inode;
//        sfs_write_block(&frame, frame_bid);
    }
    else do {
        sfs_read_block(&frame, frame_bid);
        frame_bid = frame.next;
    } while (frame_bid != 0);
    blkid temp;
    for(i = 1; i < j; i++){
        temp = sfs_alloc_block();
        frame.next = temp;
        int index;
        for(index = 0; index < SFS_FRAME_COUNT; index++){
            frame.content[index] = 0;
        }
        sfs_write_block(&frame, frame_bid);
        frame_bid = temp;
    }
    frame.next = 0;
    int index;
    for(index = 0; index < SFS_FRAME_COUNT; index++){
        frame.content[index] = 0;
    }
    sfs_write_block(&frame, frame_bid);

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
    blkid temp;

	/* TODO: find blocks between start and end.
	   Transverse the frame list if needed
	*/
    start = cur / BLOCK_SIZE;
    end = (cur + length) / BLOCK_SIZE;
    temp = fdtable[fd].inode.first_frame;
    sfs_read_block(&frame, temp);
    int ii;
    
    for(ii = 0; ii < cur / (BLOCK_SIZE * SFS_FRAME_COUNT); ii++){
        temp = frame.next;
        sfs_read_block(&frame, temp);
//        //printf("HERE\n");
    }
    //printf("II: %d\n", ii);
    ii = 0;
//    ////printf("START: %d, FINISH: %d\n", start, end);
    for(i = start; i <= end; i++){
        if(frame.content[i % SFS_FRAME_COUNT] == 0){
            frame.content[i % SFS_FRAME_COUNT] = sfs_alloc_block();
        }
        *(bids + ii) = frame.content[i % SFS_FRAME_COUNT];
//        //printf("BIDS: %d\n", frame.content[i % SFS_FRAME_COUNT]);
        
        ii++;
    }
    sfs_write_block(&frame, temp);
//    //printf("TEMP: %d\n", temp);
	return ii;
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
        int i = 0;
        //printf("dir_name: %s\n  dir_next: %d", dir.dir_name, sb.first_dir);
        while(dir.next_dir != 0 && i < 10){
            dir_bid = dir.next_dir;
            sfs_read_block(&dir, dir_bid);
            i++;
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
    //printf("Magic: %d, nblocks: %d, nfreemap_blocks: %d, first_dir: %d\n", sb.magic, sb.nblocks, sb.nfreemap_blocks, sb.first_dir);
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
        if(dirRead.inodes[i] > 0){
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
        i++;
        sfs_read_block(&dir, dir.next_dir);
    }
	return i;
}

/*
 * Open a file. If it does not exist, create a new one.
 * Allocate a file desriptor for the opened file and return the fd.
 */
int sfs_open(char *dirname, char *name)
{
	blkid dir_bid = 0, inode_bid = 0;
	sfs_inode_t inode;
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
        inode_bid = dir.inodes[i];
        if(inode_bid > 2){
            sfs_read_block(&inode, inode_bid);
            if(strcmp(name, inode.file_name) == 0){
                fdtable[fd].inode = inode;
                fdtable[fd].inode_bid = inode_bid;
                fdtable[fd].dir_bid = dir_bid;
                fdtable[fd].cur = 0;
                fdtable[fd].valid = 1;
                return fd;
            }
        }
        else if(free == -1){
            free = i;
        }
    }
    
    if(free == -1)
        return -1;
    
	/* TODO: create a new file */
    inode_bid = sfs_alloc_block();
    inode.size = 0;
    inode.first_frame = 0;
    strcpy(inode.file_name, name);
    sfs_write_block(&inode, inode_bid);
    dir.inodes[free] = inode_bid;
    sfs_write_block(&dir, dir_bid);
    
    fdtable[fd].inode = inode;
    fdtable[fd].inode_bid = inode_bid;
    fdtable[fd].dir_bid = dir_bid;
    fdtable[fd].cur = 0;
    fdtable[fd].valid = 1;
    
	return fd;
}

/*
 * Close a file. Just mark the valid field to be zero.
 */
int sfs_close(int fd)
{
	/* TODO: mark the valid field */
    fdtable[fd].valid = 0;
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
    sfs_read_block(&dir, fdtable[fd].dir_bid);
    for(i = 0; i < SFS_DB_NINODES; i++){
        blkid inode_bid = dir.inodes[i];
        if(inode_bid == fdtable[fd].inode_bid){
            dir.inodes[i] = 0;
            sfs_write_block(&dir, fdtable[fd].dir_bid);
            break;
        }
    }
    

	/* TODO: free inode and all its frames */
    sfs_free_block(fdtable[fd].inode_bid);
    frame_bid = fdtable[fd].inode.first_frame;
    if(frame_bid != 0){
        sfs_inode_frame_t frame;
        do {
            sfs_free_block(frame_bid);
            sfs_read_block(&frame, frame_bid);
            frame_bid = frame.next;
        } while (frame_bid != 0);
    }
    
    
	/* TODO: close the file */
    sfs_close(fd);
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
        //printf("%s\n", dir.dir_name);
        int i;
        for(i = 0; i < SFS_DB_NINODES; i++){
            sfs_inode_t inode;
            blkid inode_bid = dir.inodes[i];
            if(inode_bid > 2){
                sfs_read_block(&inode, inode_bid);
                //printf("\t%s\n", inode.file_name);
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
//    sfs_inode_t inode = fdtable[fd].inode;
    if(cur + length > fdtable[fd].inode.size){
        sfs_resize_file(fd, cur + length);
        fdtable[fd].inode.size = cur + length;
    }
	
	/* TODO: get the block ids of all contents (using sfs_get_file_content() */
    n = (cur + length) / BLOCK_SIZE + 1;
    bids = (int *)malloc(n);
    sfs_get_file_content(bids, fd, cur, length);
	/* TODO: main loop, go through every block, copy the necessary parts
	   to the buffer, consult the hint in the document. Do not forget to 
	   flush to the disk.
	*/
    int length_left = length;
//    //printf("FILE: %s\n", fdtable[fd].inode_bid);
    for(i = 0; i < n; i++){
        if(i == 0){
            sfs_read_block(&tmp, *(bids));
            //printf("WRITE BEFORE: %s %d\n", tmp, *(bids));
            memcpy(&(tmp[cur % BLOCK_SIZE]), p, (cur + length) % BLOCK_SIZE);
            sfs_write_block(&tmp, *(bids));
            //printf("WRITE: %s\n", tmp);
            length_left = length - ((cur + length) % BLOCK_SIZE);
        }
        else{
            sfs_write_block(&tmp, *(bids + i));
            memcpy(&tmp, (p + length - length_left), BLOCK_SIZE);
            sfs_write_block(&tmp, *(bids + i));
            length_left = length_left - BLOCK_SIZE;
        }
//        printf("WRITE: %s\n", tmp);
    }
    
    
	/* TODO: update the cursor and free the temp buffer
	   for sfs_get_file_content()
	*/
    fdtable[fd].cur = cur + length;
    free(bids);
    printf("cur: %d\n", fdtable[fd].cur);
//    fdtable[fd].inode = inode;
    
	return length;
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
    n = (cur + length) / BLOCK_SIZE + 1;
    bids = (int *)malloc(n);
    u32 test = sfs_get_file_content(bids, fd, cur, length);
    
    int length_left = length;
    
    for(i = 0; i < n; i++){
        if(i == 0){
            sfs_read_block(&tmp, *(bids));
            memcpy(p, &(tmp[cur % BLOCK_SIZE]), (cur + length) % BLOCK_SIZE);
            //printf("READ: %s\n", p);
            length_left = length - ((cur + length) % BLOCK_SIZE);
        }
        else{
            sfs_write_block(&tmp, *(bids + i));
            memcpy((p + length - length_left), &tmp, BLOCK_SIZE);
            length_left = length_left - BLOCK_SIZE;
        }
    }
    fdtable[fd].cur = cur + length;
    free(bids);
    
    
	return length;
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
    switch (loc) {
        case SFS_SEEK_SET:
            fdtable[fd].cur = relative;
            printf("SET: %d\n", fdtable[fd].cur);
            break;
        case SFS_SEEK_CUR:
            fdtable[fd].cur = fdtable[fd].cur + relative;
            printf("CUR: %d\n", fdtable[fd].cur);
            break;
        case SFS_SEEK_END:
            fdtable[fd].cur = fdtable[fd].inode.size + relative;
            printf("END: %d\n", fdtable[fd].cur);
            break;
    }
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
    if(fdtable[fd].inode.size < fdtable[fd].cur)
//        return 1;
	return 0;
}
