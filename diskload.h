#ifndef __DISKLOAD_H__
#define __DISKLOAD_H__

// disk load global variables
extern unsigned char *disk;
extern struct ext2_super_block *sb;
extern struct ext2_group_desc *bgd;

// load disk function
extern int loaddisk(char* disk_path);

#endif
