#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include "diskload.h"
#include "ext2.h"
#include "utils.h"


int main(int argc, char **argv) {
  if (argc < 4 || argc > 5) {
    fprintf(stderr, "Usage: ./ext2_ln <image file path> [-s] <absolute target path> <absolute destination path>\n");
    exit(1);
  }
  loaddisk(argv[1]);
  char *target;
  char *dest;
  unsigned char target_ft;

  if (argc == 4) {
    // Target file (absolute path)
    target = argv[2];
    // Destination (absolute) path
    dest = argv[3];
    target_ft = EXT2_FT_REG_FILE;
  }
  else {
    // If there are 5 args given, the second arg should be "-s"
    if (strcmp(argv[2], "-s") != 0) {
        fprintf(stderr, "Usage: ./ext2_ln <image file path> [-s] <absolute target path> <absolute destination path>\n");
        exit(1);
    }
    target = argv[3];
    dest = argv[4];
    target_ft = EXT2_FT_SYMLINK;
}
  // Check that target is a file (not a directory)
  struct ext2_dir_entry_2 *target_dir_entry;
  struct ext2_inode *target_inode = path_walk(target, &target_dir_entry);
  char *orig_filename = basename(target);
  // Check that given target file is valid and not a directory
  if (target_inode == NULL) {
    printf("No such file or directory\n");
    return ENOENT;
  } else if (get_type_inode(target_inode) == EXT2_FT_DIR) {
    printf("Target must be a file\n");
    return EISDIR;
  }

  char* last_section_name;
  int second_last_inodenum;
  // Perform path walk to directory one above destination path
  struct ext2_inode *second_last = path_walk_second_last(
            dest, &last_section_name, &second_last_inodenum);
  // validate if given destination path was valid
  if (second_last == NULL) {
    printf("No such file or directory\n");
    return ENOENT;
  }
  char* des_filename;
  struct ext2_inode *des_dir = second_last;
  // if destination is root
   if (last_section_name == NULL) des_filename = orig_filename;
  // validate the last_section_name: it could either be a directory or empty
  // if empty: we want to cp file to second_last_dir with new file name last_section_name
  // if it's a directory: we want to copy to that directory with same file name
  else {
    struct ext2_inode *last_inode = get_next_inode(second_last, last_section_name, NULL);
    if (last_inode == NULL) des_filename = last_section_name;
    else if (get_type_inode(last_inode) == EXT2_FT_DIR) {
      des_dir = last_inode;
      des_filename = orig_filename;
    } else {
      printf("File/link with that name already exists\n");
      exit(EEXIST);
    }
  }
  // Create hard link
  if (target_ft == EXT2_FT_REG_FILE) {
    // Create new directory entry pointing to original target's inode (target_dir_entry->inode)
    create_dir_entry(des_dir, des_filename, EXT2_FT_REG_FILE, target_dir_entry->inode);
  }
  // Create symbolic link
  else {
    struct ext2_dir_entry_2 *new_dir_entry = create_dir_entry(
              des_dir, des_filename, EXT2_FT_SYMLINK, 0);

    // Data block for new inode should contain the path to the original file (target)
    struct ext2_inode *new_inode = get_inode(new_dir_entry->inode);

    // Allocate block, update inode block information
    int blocknum = allocate_block();
    unsigned char* block = get_block(blocknum);

    // Length of path to original target file (also, size of symlink)
    unsigned int target_path_len = strlen(target);

    new_inode->i_block[0] = blocknum;
    new_inode->i_blocks = calculate_iblocks(new_inode->i_blocks, EXT2_BLOCK_SIZE);
    new_inode->i_size = target_path_len;

    // Copy the target path into the block
    memcpy(block, target, target_path_len);
  }
  return 0;
}
