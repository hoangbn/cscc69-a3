#ifndef __UTILS_H__
#define __UTILS_H__

// debuger helpers
extern void print_inode(struct ext2_inode *inode);
extern void print_disk_image();

// utils
extern void remove_directory(struct ext2_inode *parent_inode);
extern void unlink_inode(unsigned int inodenum);
extern void remove_dir_entry(struct ext2_inode *parent_inode, char* name);
extern struct ext2_dir_entry_2 *create_dir_entry(struct ext2_inode *parent_inode,
                       char* name, char file_type, unsigned int inodenum);
extern struct ext2_inode *path_walk(char *path, struct ext2_dir_entry_2 **dir_entry);
struct ext2_inode *path_walk_second_last(char *path, char** last_section_name, int* inodenum);
extern struct ext2_inode *get_root_dir();
extern struct ext2_inode *get_next_dir(struct ext2_inode *cur_dir, char *dir_name, int* inodenum);
extern struct ext2_inode *get_next_inode(struct ext2_inode *cur_dir, char *name, int* inodenum);
extern struct ext2_dir_entry_2 *get_next_dir_entry(struct ext2_inode *cur_dir, char *name);
extern unsigned char get_type_inode(struct ext2_inode *inode);
extern struct ext2_inode *get_inode(int inodenum);
extern unsigned char *get_block(int blocknum);
extern unsigned long get_block_pos(int blocknum);
extern int is_cur_or_prev (char* name, int name_len);

// local helper function
void path_as_array(char *path, char **path_array, int *sections_count);
int real_rec_len_round(int real_rec_len);
int get_inode_mode(char file_type);
int allocate_inode(char file_type);
void init_entry_values(struct ext2_dir_entry_2 *entry, unsigned int inodenum, unsigned short rec_len,
                        unsigned char name_len, unsigned char file_type, char *name);
int allocate_block();
int get_inode_mode(char file_type);
char *get_block_bitmap();
char *get_inode_bitmap();
unsigned int calculate_iblocks(unsigned int old_iblocks, int extra_bytes);
void free_block(unsigned int blocknum);
void free_inode(unsigned int inodenum);

#endif
