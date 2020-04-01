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
  if (argc < 3) {
    fprintf(stderr, "Usage: ./ext2_mkdir <image file path> <absolute path to new directory>\n");
    exit(1);
  }
  // load disk and perform path walk to second to last directory
  loaddisk(argv[1]);
  print_disk_image(); // TODO: remove
  char* last_section_name;
  int second_last_inodenum;
  struct ext2_inode *second_last = path_walk_second_last(argv[2],
                                    &last_section_name, &second_last_inodenum);
  // validate if given path was valid
  if (second_last == NULL) {
    printf("No such file or directory\n");
    return ENOENT;
  }
  if (last_section_name == NULL) {
    printf("Can't create root directory\n");
    return EEXIST;
  }
  // create new entry for new directory
  struct ext2_dir_entry_2 *new_dir_entry = create_dir_entry(
            second_last, last_section_name, EXT2_FT_DIR, 0);
  struct ext2_inode *new_dir_inode = get_inode(new_dir_entry->inode);
  // update count of directories
  bgd->bg_used_dirs_count++;
  // create new entries for this new directory (. and ..)
  create_dir_entry(new_dir_inode, ".", EXT2_FT_DIR, new_dir_entry->inode);
  create_dir_entry(new_dir_inode, "..", EXT2_FT_DIR, second_last_inodenum);

  print_disk_image(); // TODO: remove
  return 0;
}
