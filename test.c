#include "ext.h"
#include <stdio.h>
#include <string.h>

static void compiler_validation()
{
	if (sizeof(u32) != 4) {
		printf("sizeof(u32)!=4\n");
		return ;
	}
	if (sizeof(u16) != 2) {
		printf("sizeof(u16)!=2\n");
		return ;
	}
	if (sizeof(u8) != 1) {
		printf("sizeof(u8)!=1\n");
		return ;
	}
	if (sizeof(sfs_inode_t) != BLOCK_SIZE) {
		printf("inode size not aligned to BLOCK_SIZE!\n");
		return ;
	}
	if (sizeof(sfs_superblock_t) != BLOCK_SIZE) {
		printf("superblock size not aligned to BLOCK_SIZE!\n");
		return ;
	}
	if (sizeof(sfs_inode_frame_t) != BLOCK_SIZE) {
		printf("frame size not aligned to BLOCK_SIZE!\n");
		return ;
	}
}

static u8 tmpbuf[BLOCK_SIZE];

static int testcase1(void)
{
	/* create SFS and load the superblock */
	sfs_superblock_t *p;
	sfs_mkfs();
	p = sfs_print_info();
	return !(p->nblocks == 1024 && p->nfreemap_blocks == 1);
}

static int testcase2(void)
{
	/* create and remove directories */
	sfs_mkdir("root");
	sfs_mkdir("haha");
	if (sfs_lsdir() != 2)
		return 1;
	sfs_mkdir("wawa");
	printf("--------------\n");
	if (sfs_lsdir() != 3)
		return 1;
	sfs_rmdir("haha");
	printf("--------------\n");
	if (sfs_lsdir() != 2)
		return 1;
	sfs_rmdir("wawa");
	sfs_rmdir("root");
	printf("--------------\n");
	if (sfs_lsdir() != 0)
		return 1;
	sfs_mkdir("root");
	return 0;
}

static int testcase3(void)
{
	/* create and remove files*/
	int fd, fd2, fd3;
	fd = sfs_open("root","file1");
	if (sfs_ls() != 1) {
		return 1;
	}
	fd2 = sfs_open("root","file1-1");
	fd3 = sfs_open("root","file1-2");
	if (sfs_ls() != 3) {
		return 1;
	}
	sfs_remove(fd2);
	sfs_remove(fd);
	if (sfs_ls() != 1) {
		return 1;
	}
	sfs_remove(fd3);
	if (sfs_ls() != 0) {
		return 1;
	}
	return 0;
}

static int testcase4(void)
{
	/* write,close,open,read */
	int tmp;
	int fd;
	fd = sfs_open("root","file2");
	tmp = sfs_write(fd, "hello world!", 13);
	sfs_close(fd);
	if (tmp != 13) {
		return 1;
	}
	fd = sfs_open("root", "file2");
	memset(tmpbuf,0,BLOCK_SIZE);
	tmp = sfs_read(fd, tmpbuf, 13);
	sfs_close(fd);
	if (tmp != 13) {
		return 1;
	}
	return memcmp("hello world!", tmpbuf, 13);
}

static int testcase5(void)
{
	/* write, seek back and read */
	int tmp;
	int fd;
	fd = sfs_open("root", "file3");
	tmp = sfs_write(fd, "hello world!", 13);
	if (tmp != 13) {
		return 1;
	}
	sfs_seek(fd, -7, SFS_SEEK_CUR);
	memset(tmpbuf,0,BLOCK_SIZE);
	tmp = sfs_read(fd, tmpbuf, 7);
	sfs_close(fd);
	if (tmp != 7) {
		return 1;
	}
	return memcmp("world!", tmpbuf, 7);
}

static int testcase6(void)
{
	/* write, seek back and read to eof */
	int tmp;
	int fd;
	int i;
	fd = sfs_open("root","file4");
	tmp = sfs_write(fd, "hello world!", 13);
	if (tmp != 13) {
		return 1;
	}
	sfs_seek(fd, -7, SFS_SEEK_CUR);
	memset(tmpbuf,0,BLOCK_SIZE);
	for (i = 0; i < BLOCK_SIZE && !sfs_eof(fd); ++i) {
		tmp = sfs_read(fd, &tmpbuf[i], 1);
		if (tmp != 1)
			return 1;
	}
	sfs_close(fd);
	return memcmp("world!", tmpbuf, 7);
}

static int testcase7(void)
{
	/* write, seek back and overwrite */
	int tmp;
	int fd;
	fd = sfs_open("root","file5");
	tmp = sfs_write(fd, "hello world!!", 14);
	if (tmp != 14) {
		return 1;
	}
	sfs_seek(fd, -8, SFS_SEEK_CUR);
	sfs_write(fd, "comp310", 7);
	sfs_seek(fd, 0, SFS_SEEK_SET);
	memset(tmpbuf,0,BLOCK_SIZE);
	tmp = sfs_read(fd, tmpbuf, 14);
	if (tmp != 14)
		return 1;
	sfs_close(fd);
	return memcmp("hello comp310", tmpbuf, 14);
}

static int testcase8(void)
{
	/* write, seek back and write to increase size */
	int tmp;
	int fd;
	fd = sfs_open("root","file6");
	sfs_write(fd, "hello world!!", 14);
	sfs_seek(fd, -8, SFS_SEEK_CUR);
	sfs_write(fd, "comp310", 7);
	sfs_write(fd, ". Assignment 2 is a ", 20);
	sfs_write(fd, "hard work but good experience", 30);
	sfs_seek(fd, 0, SFS_SEEK_SET);
	memset(tmpbuf,0,BLOCK_SIZE);
	tmp = sfs_read(fd, tmpbuf, 63);
	if (tmp != 63)
		return 1;
	sfs_close(fd);
	return memcmp(tmpbuf,
		"hello comp310. Assignment 2 is a hard work but good experience", 63);
}

static int testcase9(void)
{
	/* complicated seek */
	int tmp;
	int fd;
	fd = sfs_open("root","file7");
	sfs_write(fd, "xx", 2);
	sfs_seek(fd, +4, SFS_SEEK_CUR);
	sfs_write(fd, "world", 5);
	sfs_seek(fd, -6, SFS_SEEK_END);
	sfs_write(fd, " ", 1);
	sfs_seek(fd, 11, SFS_SEEK_SET);
	sfs_write(fd, "!", 2);
	sfs_seek(fd, -11, SFS_SEEK_END);
	sfs_seek(fd, -2, SFS_SEEK_CUR);
	sfs_write(fd, "hello", 5);
	sfs_seek(fd, 0, SFS_SEEK_SET);
	memset(tmpbuf, 0, BLOCK_SIZE);
	tmp = sfs_read(fd, tmpbuf, 13);
	if (tmp != 13)
		return 1;
	sfs_close(fd);
	return memcmp("hello world!", tmpbuf, 13);
}

static int testcase10(void)
{
	/* large file */
	int fd;
	int tmp;
	int i;

	fd = sfs_open("root","file3");
	for (i = 0; i < 8000; ++i) {
		tmp = sfs_write(fd, "hello world", 12);
		if (tmp != 12)
			return 1;
	}
	sfs_seek(fd, 0, SFS_SEEK_SET);
	for (i = 0; i < 8000; ++i) {
		tmp = sfs_read(fd, tmpbuf, 12);
		if (tmp != 12)
			return 1;
	}
	sfs_close(fd);
	return 0;
}

static int (*tests[])(void) = {
	testcase1, testcase2, testcase3, testcase4,
	testcase5, testcase6, testcase7, testcase8,
	testcase9, testcase10,
};

int main()
{
	int i;

	compiler_validation();
	sfs_init_storage();

	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
		if (tests[i]()) {
			printf("%d: FAILED\n", i+1);
			break;
		}
		printf("%d: PASS\n", i+1);
	}

	sfs_close_storage();
	return 0;
}
