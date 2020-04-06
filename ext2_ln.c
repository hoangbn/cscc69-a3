#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include "diskload.h"
#include "ext2.h"
#include "utils.h"


int main(int argc, char **argv) {
  // 
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
      if (argv[2] != "-s") {
          fprintf(stderr, "Usage: ./ext2_ln <image file path> [-s] <absolute target path> <absolute destination path>\n");
          exit(1);
      }
      target = argv[3];
      dest = argv[4];
      target_ft = EXT2_FT_SYMLINK;
  }

  char *dest_filename = basename(dest);

  // Check that target is a file (not a directory)
  struct ext2_dir_entry_2 *target_dir_entry;
  struct ext2_inode *target_inode = path_walk(target, &target_dir_entry);

  // Check that given target file is valid and not a directory
  if (target_inode == NULL) {
    return ENOENT;
  } else if (get_type_inode(target_inode) == EXT2_FT_DIR) {
    return EISDIR;
  }

  char* last_section_name;
  int second_last_inodenum;
  // Perform path walk to directory one above destination path
  struct ext2_inode *second_last = path_walk_second_last(
            dest, &last_section_name, &second_last_inodenum);

  // Create hard link
  if (target_ft == EXT2_FT_REG_FILE) {
    // Create new directory entry pointing to original target's inode (target_dir_entry->inode)
    struct ext2_dir_entry_2 *new_dir_entry = create_dir_entry(
              second_last, dest_filename, EXT2_FT_REG_FILE, target_dir_entry->inode);
  }
  // Create symbolic link
  else {
    struct ext2_dir_entry_2 *new_dir_entry = create_dir_entry(
              second_last, dest_filename, EXT2_FT_SYMLINK, 0);

    // TODO: Data block for new inode should contain the path to the original file (target)
  }

}
