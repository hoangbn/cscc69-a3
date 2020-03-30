#ifndef __DISKLOAD_H__
#define __DISKLOAD_H__

extern unsigned char *disk;
extern struct ext2_super_block *sb;
extern struct ext2_group_desc *bgd;

extern int loaddisk(char *disk_path);

#endif
