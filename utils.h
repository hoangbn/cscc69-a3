#ifndef __UTILS_H__
#define __UTILS_H__

// debuger helpers
extern void print_inode(struct ext2_inode* inode);

// utils
extern struct ext2_inode* path_walk(char* path);

void path_as_array(char* path, char** path_array, int* sections_count);
struct ext2_inode* get_root_dir();
struct ext2_inode* get_next_dir(struct ext2_inode* cur_dir, char* dir_name);
struct ext2_inode* get_next_inode(struct ext2_inode* cur_dir, char* name);
char get_type(struct ext2_inode* inode);
struct ext2_inode* get_inode(int inodenum);

#endif