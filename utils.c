#include <errno.h>
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

// debugging helper
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

// get inode at given path(can be a directory or a file), and corresponding
// dir_entry value to dir_entry parameter. If not valid return NULL
struct ext2_inode *path_walk(char *path, struct ext2_dir_entry_2 **dir_entry) {
  char *last_section_name;
  struct ext2_inode *second_last = path_walk_second_last(path, &last_section_name);
  if (second_last == NULL) return NULL;
  struct ext2_dir_entry_2 *last = get_next_dir_entry(second_last, last_section_name);
  if (dir_entry != NULL) *dir_entry = last;
  return last == NULL ? NULL : get_inode(last->inode);
}

// get inode of a second to last section on path (must be a directory since
// there is also the last section). Also assigns name of last section.
// Returns NULL if path not valid
struct ext2_inode *path_walk_second_last(char *path, char** last_section_name) {
  if (path[0] != '/')
    return NULL;
  // divide path into sections, store in array
  int sections_count;
  char *path_array[100];
  path_as_array(path, path_array, &sections_count);
  // get the root directory
  struct ext2_inode *cur = get_root_dir();
  if (sections_count == 0)
    return cur;
  // walk through path until last section
  for (int i = 0; i < sections_count - 1; i++) {
    cur = get_next_dir(cur, path_array[i]);
    if (cur == NULL)
      return NULL;
  }
  if (last_section_name != NULL) 
    *last_section_name = path_array[sections_count - 1];
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

// get next directory given it's name and inode of it's parent directory, return
// null if doesn't exist
struct ext2_inode *get_next_dir(struct ext2_inode *cur_dir, char *dir_name) {
  // get inode with given name, check if it's a directory, return accordingly
  struct ext2_inode *next_inode = get_next_inode(cur_dir, dir_name);
  if (next_inode == NULL || get_type_inode(next_inode) != 'd')
    return NULL;
  return next_inode;
}

// get next inode from given name and inode of it's parent directory, return
// null if doesn't exist
struct ext2_inode *get_next_inode(struct ext2_inode *cur_dir, char *name) {
  struct ext2_dir_entry_2 *dir = get_next_dir_entry(cur_dir, name);
  return dir == NULL ? NULL : get_inode(dir->inode);
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
char get_type_inode(struct ext2_inode *inode) {
  return S_ISDIR(inode->i_mode) ? 'd' : (S_ISREG(inode->i_mode) ? 'f' : 's');
}

// get type of dir entry
char get_type_dir_entry(struct ext2_dir_entry_2 *dir) {
  return dir->file_type == EXT2_FT_REG_FILE ? 'f' : 
                        (dir->file_type == EXT2_FT_DIR ? 'd' : 's');
}

// get inode from inode number
struct ext2_inode *get_inode(int inodenum) {
  return ((struct ext2_inode *)(disk + bgd->bg_inode_table * EXT2_BLOCK_SIZE)) +
         inodenum - 1;
}
