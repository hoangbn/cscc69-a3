#include <errno.h>
#include <time.h> 
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "ext2.h"
#include "utils.h"
#include "diskload.h"

// debugging helpers
void print_inode(struct ext2_inode *inode) {
  if (inode == NULL) {
    printf("Path Invalid\n");
    return;
  }
  printf("[current] type: %c size: %d links: %d blocks: %d\n", get_type_inode(inode),
         inode->i_size, inode->i_links_count, inode->i_blocks);
  // Now to print all the blocks for the inode
  printf("[current] Blocks: ");
  // Get the array of blocks from inode
  unsigned int *arr = inode->i_block;
  // Loop through and print all value till a 0 is seen in the array
  while (1) {
    if (*arr == 0)
      break;
    printf("%d ", *arr++);
  }
  printf("\n");
}

void print_disk_image() {
  printf("Inodes: %d\n", sb->s_inodes_count);
  printf("Blocks: %d\n", sb->s_blocks_count);
  printf("Block group:\n");

  printf("    block bitmap: %d\n", bgd->bg_block_bitmap);
  printf("    inode bitmap: %d\n", bgd->bg_inode_bitmap);
  printf("    inode table: %d\n", bgd->bg_inode_table);
  printf("    free blocks: %d\n", bgd->bg_free_blocks_count);
  printf("    free inodes: %d\n", bgd->bg_free_inodes_count);
  printf("    used_dirs: %d\n", bgd->bg_used_dirs_count);
  // Get block bitmap
  char *bm = (char *) (disk + (bgd->bg_block_bitmap * EXT2_BLOCK_SIZE));
  // counter for shift
  printf("block bitmap: ");
  int index = 0;
  for (int i = 0; i < sb->s_blocks_count; i++) {
    unsigned c = bm[i / 8];                     // get the corresponding byte
    printf("%d", (c & (1 << index)) > 0);       // Print the correcponding bit
    if (++index == 8) (index = 0, printf(" ")); // increment shift index, if > 8 reset.
  }
  printf("\n");
  // Get inode bitmap
  char *bmi = (char *) (disk + (bgd->bg_inode_bitmap * EXT2_BLOCK_SIZE));
  // Want to keep track of the used inodes in this array
  int inum[32];
  inum[0] = 1;    // Root inode is Inode number 2, index 1
  // current size of the array
  int inumc = 1;  // because we stored the first one
  // counter for shift
  printf("inode bitmap: ");
  int index2 = 0;
  for (int i = 0; i < sb->s_inodes_count; i++) {
      unsigned c = bmi[i / 8];                     // get the corresponding byte
      printf("%d", (c & (1 << index2)) > 0);       // Print the correcponding bit
      // If that bit was a 1, inode is used, store it into the array.
      // Note, this is the index number, NOT the inode number
      // inode number = index number + 1
      if ((c & (1 << index2)) > 0 && i > 10) {    // > 10 because first 11 not used
          inum[inumc++] = i;
      }
      if (++index2 == 8) (index2 = 0, printf(" ")); // increment shift index, if > 8 reset.
  }
  printf("\n\n");

  struct ext2_inode* in = (struct ext2_inode*) (disk + bgd->bg_inode_table * EXT2_BLOCK_SIZE);

  /******************************* Print Inodes *************************************/
  /**** The following array is used to keep track of directories ****/
  int dirs[128];
  int dirsin = 0;

  printf("Inodes:\n");

  // Go through all the used inodes stored in the array above
  for (int i = 0; i < inumc; i++) {
      printf("%d: \n", i);
      // Remember array stores the index
      struct ext2_inode* curr = in + inum[i];
      int inodenum = inum[i] + 1;     // Number = index + 1
      char type = (S_ISDIR(curr->i_mode)) ? 'd' : ((S_ISREG(curr->i_mode)) ? 'f' : 's');
      printf("[%d] type: %c size: %d links: %d blocks: %d\n", inodenum, type, \
          curr->i_size, curr->i_links_count, curr->i_blocks);     // Print Inode info
      
      // Now to print all the blocks for the inode
      printf("[%d] Blocks: ", inodenum);  
      // Get the array of blocks from inode
      unsigned int *arr = curr->i_block;
      // Loop through and print all value till a 0 is seen in the array
      while(1) {
          if (*arr == 0) {
              break;
          }
          // If it's a directory, add to the array.
          if (type == 'd') {
              dirs[dirsin++] = *arr;
          }
          printf("%d ", *arr++);
      }
      printf("\n");
  }
  printf("\n");

  printf("Directory Blocks:\n");
  // For all the directory blocks
  for (int i = 0; i < dirsin; i++) {
    // Get the block number
    int blocknum = dirs[i];
    // Get the position in bytes and index to block
    unsigned long pos = (unsigned long) disk + blocknum * EXT2_BLOCK_SIZE;
    struct ext2_dir_entry_2 *dir = (struct ext2_dir_entry_2 *) pos;

    printf("    DIR BLOCK NUM: %d (for inode %d)\n", blocknum, dir->inode);

    do {
        // Get the length of the current block and type
        int cur_len = dir->rec_len;
        char typ = (dir->file_type == EXT2_FT_REG_FILE) ? 'f' : 
                    ((dir->file_type == EXT2_FT_DIR) ? 'd' : 's');
        // Print the current directory entry
        printf("Inode: %d rec_len: %d name_len: %d file_type: %c name: %.*s\n", 
            dir->inode, dir->rec_len, dir->name_len, typ, dir->name_len, dir->name);
        // Update position and index into it
        pos = pos + cur_len;
        dir = (struct ext2_dir_entry_2 *) pos;

        // Last directory entry leads to the end of block. Check if 
        // Position is multiple of block size, means we have reached the end
    } while (pos % EXT2_BLOCK_SIZE != 0);
  }
}

// attempts to create a new directory entry with given name and file type, sets inode
// of new entry to given inode number. If given inode number is 0, allocates a new inode
struct ext2_dir_entry_2 *create_dir_entry(struct ext2_inode *parent_inode,
                       char* name, char file_type, unsigned int inodenum) {
  // check if there already exists an entry with given name
  if (get_next_dir_entry(parent_inode, name) != NULL) exit(EEXIST);
  // get inode number to set for new entry
  unsigned int entry_inode = inodenum == 0 ? allocate_inode(file_type) : inodenum;
  int dir_entry_size = sizeof(struct ext2_dir_entry_2);
  int name_len = strlen(name);
  // get actual rec_len needed for new entry (rounded up to nearest multiple of 4)
  int needed_rec_len = real_rec_len_round(dir_entry_size + name_len);
  // start looping through blocks, computing actual size of directory entries
  // in the blocks to see if new entry can fit in between
  unsigned int *arr = parent_inode->i_block;
  while (*arr != 0) {
    // get starting position in the block;
    unsigned long pos = (unsigned long)disk + *arr * EXT2_BLOCK_SIZE;
    struct ext2_dir_entry_2 *dir = (struct ext2_dir_entry_2 *)pos;
    // loop till the end of the block, checking if there is space for new entry
    do {
      // get real rec_len of current entry
      int cur_real_len = real_rec_len_round(dir_entry_size + dir->name_len);
      // if there is enough space for new entry
      int free_len = dir->rec_len - cur_real_len;
      if (free_len >= needed_rec_len) {
        // create new entry with new values and return it
        pos += cur_real_len;
        struct ext2_dir_entry_2 *new_entry = (struct ext2_dir_entry_2 *)pos;
        init_entry_values(new_entry, entry_inode, free_len, name_len, file_type, name);
        return new_entry;
      }
      // advance to the next inode
      pos += dir->rec_len;  
      dir = (struct ext2_dir_entry_2 *)pos;
    } while (pos % EXT2_BLOCK_SIZE != 0);
    // advance to the next block in array
    arr++;
  }
  // at this point, all blocks are full, we need to allocate a new block
  // Note: by assignment description, we only need to handle single indirection for files
  // and more than one block for directory (no need for single indirection for directories)
  // so assuming here, *arr value < 12;
  int blocknum = allocate_block();
  // update inode after allocating
  *arr = blocknum;
  parent_inode->i_size = parent_inode->i_size + EXT2_BLOCK_SIZE;
  parent_inode->i_blocks = calculate_iblocks(parent_inode->i_blocks, EXT2_BLOCK_SIZE);
  // get the block and create new entry (offset 0)
  unsigned long new_block_pos = (unsigned long)disk + blocknum * EXT2_BLOCK_SIZE;
  struct ext2_dir_entry_2 *new_entry = (struct ext2_dir_entry_2 *) new_block_pos;
  init_entry_values(new_entry, entry_inode, EXT2_BLOCK_SIZE, name_len, file_type, name);
  return new_entry;
}

// initialize directory entry values
void init_entry_values(struct ext2_dir_entry_2 *entry, unsigned int inodenum, unsigned short rec_len,
                        unsigned char name_len, unsigned char file_type, char *name) {
  // initializing entry values
  entry->inode = inodenum;
  entry->rec_len = rec_len;
  entry->name_len = name_len;
  entry->file_type= file_type;
  strncpy(entry->name, name, name_len);
  // updating link count of inode that this entry links to
  get_inode(inodenum)->i_links_count++;
}

// get inode at given path(can be a directory or a file), and corresponding
// dir_entry value to dir_entry parameter. If not valid return NULL
struct ext2_inode *path_walk(char *path, struct ext2_dir_entry_2 **dir_entry) {
  char *last_section_name;
  struct ext2_inode *second_last = path_walk_second_last(path, &last_section_name, NULL);
  if (second_last == NULL) return NULL;
  if (last_section_name == NULL) return second_last;
  struct ext2_dir_entry_2 *last = get_next_dir_entry(second_last, last_section_name);
  if (dir_entry != NULL) *dir_entry = last;
  return last == NULL ? NULL : get_inode(last->inode);
}

// get inode of a second to last section on path (must be a directory since
// there is also the last section). Also assigns name of last section.
// Returns NULL if path not valid
struct ext2_inode *path_walk_second_last(char *path, char** last_section_name, int* inodenum) {
  if (path[0] != '/')
    return NULL;
  // divide path into sections, store in array
  int sections_count;
  char *path_array[100];
  path_as_array(path, path_array, &sections_count);
  // get the root directory
  struct ext2_inode *cur = get_root_dir();
  if (sections_count == 0) {
    if (last_section_name != NULL) *last_section_name = NULL;
    return cur;
  }
  // walk through path until last section
  for (int i = 0; i < sections_count - 1; i++) {
    cur = get_next_dir(cur, path_array[i], inodenum);
    if (cur == NULL)
      return NULL;
  }
  // assign last section name, and inodenumber for case we're in root dir
  if (last_section_name != NULL) 
    *last_section_name = path_array[sections_count - 1];
  if (inodenum != NULL && sections_count == 1) *inodenum = 2;
  return cur;
}

// provides sections array of the path (not including root), and count of
// sections
void path_as_array(char *path, char **path_array, int *sections_count) {
  int count = 0;
  char *modifiablePath = strdup(path);
  char *p = strtok(modifiablePath, "/");
  while (p != NULL) {
    path_array[count] = p;
    count++;
    p = strtok(NULL, "/");
  }
  *sections_count = count;
}

// get inode of root directory
struct ext2_inode *get_root_dir() {
  return get_inode(2);
}

// get next directory given it's name and inode of it's parent directory, 
// record inode number of next dir. Return null if doesn't exist
struct ext2_inode *get_next_dir(struct ext2_inode *cur_dir, char *dir_name, int* inodenum) {
  // get inode with given name, check if it's a directory, return accordingly
  struct ext2_inode *next_inode = get_next_inode(cur_dir, dir_name, inodenum);
  if (next_inode == NULL || get_type_inode(next_inode) != EXT2_FT_DIR)
    return NULL;
  return next_inode;
}

// get next inode from given name and inode of it's parent directory, 
// record inode in a given int pointer. Return null if doesn't exist
struct ext2_inode *get_next_inode(struct ext2_inode *cur_dir, char *name, int* inodenum) {
  struct ext2_dir_entry_2 *dir = get_next_dir_entry(cur_dir, name);
  if (dir == NULL) return NULL;
  if (inodenum != NULL) *inodenum = dir->inode;
  return get_inode(dir->inode);
}

// get directory entry given name and it's parent directory inode
struct ext2_dir_entry_2 *get_next_dir_entry(struct ext2_inode *cur_dir, char *name) {
  int name_len = strlen(name);
  // Get the array of blocks from inode
  unsigned int *arr = cur_dir->i_block;
  while (*arr != 0) {
    // get starting position in the block;
    unsigned long pos = (unsigned long)disk + *arr * EXT2_BLOCK_SIZE;
    struct ext2_dir_entry_2 *dir = (struct ext2_dir_entry_2 *)pos;
    // loop till the end of the block
    do {
      // if given name match current inode name, return current inode
      if (name_len == dir->name_len && strncmp(dir->name, name, dir->name_len) == 0)
        return dir;
      // advance to the next inode
      pos += dir->rec_len;  
      dir = (struct ext2_dir_entry_2 *)pos;
    } while (pos % EXT2_BLOCK_SIZE != 0);
    // advance to the next block in array
    arr++;
  }
  return NULL;
}

// get type of inode
unsigned char get_type_inode(struct ext2_inode *inode) {
  return S_ISDIR(inode->i_mode) ? EXT2_FT_DIR : 
    (S_ISREG(inode->i_mode) ? EXT2_FT_REG_FILE : EXT2_FT_SYMLINK);
}

// get inode from inode number
struct ext2_inode *get_inode(int inodenum) {
  return ((struct ext2_inode *)(disk + bgd->bg_inode_table * EXT2_BLOCK_SIZE)) +
         inodenum - 1;
}

unsigned char *get_block(int blocknum) {
  return disk + (blocknum * EXT2_BLOCK_SIZE);
}

// round up real rec_len of directory entry to nearest multiple of 4
int real_rec_len_round(int real_rec_len) {
  int remainder = real_rec_len % 4;
  return remainder == 0 ? real_rec_len : real_rec_len + 4 - remainder;
}

// find and set first low bit in bitmap to high, returns corresponding
// index number of corresponding set bit
int allocate_resource_on_bitmap(char *bm, int bitmap_size) {
  int index = 0;
  for (int i = 0; i < bitmap_size; i++) {
    // get corresponding byte
    unsigned cur_byte = bm[i / 8];
    // if corresponding bit is 0, change it to 1, return index number
    if (!((cur_byte & (1 << index)) > 0)) {
      bm[i/8] = cur_byte | (1 << index);
      return i;
    } 
    // increment shift index, if > 8 reset
    if (++index == 8) index = 0;
  }
  // if there is no low bit, there is not enought memory -> exit
  exit(ENOMEM);
}

// find unused inode, return it's number
int allocate_inode(char file_type) {
  // get the inode number (index + 1)
  int index = allocate_resource_on_bitmap(get_inode_bitmap(), sb->s_inodes_count);
  int inodenum = index + 1;
  // get inode, reset all it's values and initialize new values (timefields not needed)
  // note, direcotry link is not done here (is in init_entry_values instead)
  struct ext2_inode *inode = get_inode(inodenum);
  memset(inode, 0, sizeof(struct ext2_inode));
  inode->i_mode = get_inode_mode(file_type);
  unsigned int now = (unsigned int) time(NULL);
  inode->i_atime = now;
  inode->i_ctime = now;
  inode->i_mtime = now;
  // decrement free inode count 
  bgd->bg_free_inodes_count--;
  sb->s_free_inodes_count--;
  // return the inode number
  return inodenum;
}

// find unused inode, return it's number
int allocate_block() {
  // get the block number (index)
  int blocknum = allocate_resource_on_bitmap(get_block_bitmap(), sb->s_blocks_count);
  // get block, reset all it's bits to 0
  unsigned char *block = get_block(blocknum);
  memset(block, 0, EXT2_BLOCK_SIZE);
  // decrement free block count
  bgd->bg_free_blocks_count--;
  sb->s_free_blocks_count--;
  // return the inode number
  return blocknum;
}

// get the inode mode of given file type
int get_inode_mode(char file_type) {
  switch (file_type) {
  case EXT2_FT_DIR:
    return EXT2_S_IFDIR;
  case EXT2_FT_REG_FILE:
    return EXT2_S_IFREG;
  default:
    return EXT2_S_IFLNK;
  }
}

// get block bitmap
char *get_block_bitmap() {
  return (char *) (disk + (bgd->bg_block_bitmap * EXT2_BLOCK_SIZE));
}

// get inode bitmap
char *get_inode_bitmap() {
  return (char *) (disk + (bgd->bg_inode_bitmap * EXT2_BLOCK_SIZE));
}

// calculate iblocks (in DISK SECTIONS - 512 bytes)
unsigned int calculate_iblocks(unsigned int old_iblocks, int extra_bytes) {
  return old_iblocks + (extra_bytes / 512);
}