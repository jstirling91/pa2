#ifndef FS_H_
#define FS_H_

#define BLOCK_SIZE 512

/* Three basic types, 4 bytes, 2 bytes, and 1 byte */
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

/* define block as a 4-byte type */
typedef u32 blkid;

/* Super block */
#define SFS_SB_PADDING (BLOCK_SIZE - 4*4)
#define SFS_MAGIC 0xAABBCCDD

typedef struct _sfs_superblock
{
	u32 magic; /* a constant number */
	u32 nblocks;
	u32 nfreemap_blocks;
	blkid first_dir;
	u8 paddings[SFS_SB_PADDING];
}sfs_superblock_t;

/* Directory block */
#define SFS_DB_NINODES ((BLOCK_SIZE - 120 - 4)/sizeof(blkid))

typedef struct _sfs_dirblock
{
	blkid next_dir;
	char dir_name[120];
	blkid inodes[SFS_DB_NINODES];
}sfs_dirblock_t;


/* Inode */
#define SFS_MAX_FILENAME_LEN (BLOCK_SIZE-4*2) 
#define SFS_FRAME_COUNT 127

typedef struct _sfs_inode
{
	u32 size; /* the file size */
	blkid first_frame;
	char file_name[SFS_MAX_FILENAME_LEN];
}sfs_inode_t;

/* Frame */
typedef struct _sfs_inode_frame
{
	blkid next;
	blkid content[SFS_FRAME_COUNT];
}sfs_inode_frame_t;

/* Interfaces */
int sfs_mkfs();
sfs_superblock_t *sfs_print_info();
int sfs_mkdir(char *name);
int sfs_rmdir(char *name);
int sfs_lsdir();
int sfs_open(char *dirname, char *name);
int sfs_close(int fd);
int sfs_remove(int fd);
int sfs_ls();
int sfs_write(int fd, void *buf, int length);
int sfs_read(int fd, void *buf, int length);
#define SFS_SEEK_SET 0
#define SFS_SEEK_CUR 1
#define SFS_SEEK_END 2
int sfs_seek(int fd, int relative, int loc);
int sfs_eof(int fd);

/* File descriptor */
#define SFS_MAX_OPENED_FILES 32
typedef struct _fd_struct
{
	sfs_inode_t inode;
	blkid inode_bid, dir_bid;
	u32 cur;
	u32 valid;
}fd_struct_t;

#endif /* FS_H_ */
