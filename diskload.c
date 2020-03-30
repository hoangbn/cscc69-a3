#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;
struct ext2_super_block *sb;
struct ext2_group_desc *bgd;

int loaddisk(char *disk_path)
{
    int fd = open(disk_path, O_RDWR);
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }
    sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    bgd = (struct ext2_group_desc *)(disk + 2 * EXT2_BLOCK_SIZE);
    return 0;
}