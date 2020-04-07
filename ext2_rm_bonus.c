
#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include "diskload.h"
#include "ext2.h"
#include "utils.h"


int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: ./ext2_rm <image file path> [-r] <absolute file/link path>\n");
    exit(1);
  }
  //load disk and get the arg for sourse and destination
  loaddisk(argv[1]);
  char *rm_path = argv[2];
  int recursive = 0;
  if (strcmp(argv[2], "-r") == 0) {
    rm_path = argv[3];
    recursive = 1;
  }
  //check if file to rm exists
  char *rm_name;
  int second_last_section_inodenum;
  struct ext2_inode *second_last_dir = path_walk_second_last(rm_path,
                                    &rm_name, &second_last_section_inodenum);
  // validate if given destination path was valid
  if (second_last_dir == NULL) {
    printf("No such file or directory\n");
    return ENOENT;
  }
  // if given path is "/", give an error
  if (rm_name == NULL) {
    printf("Cannot remove root directory\n");
    return EISDIR;
  }
  // validate given path points to an existent file or link
  int rm_inodenum;
  struct ext2_inode *rm_inode = get_next_inode(second_last_dir, rm_name, &rm_inodenum);
  if (rm_inode == NULL) {
    printf("No such file or directory\n");
    return ENOENT;
  }
  unsigned char type = get_type_inode(rm_inode);
  // if not recursive, check if it's a file or link
  if (!recursive && type == EXT2_FT_DIR) {
    printf("This is a directory. Please specify a file or a link\n");
    return EISDIR;
  }
  // remove entry
  remove_dir_entry(second_last_dir, rm_name);
  // remove the directory contents (if it's a ...)
  if (recursive && type == EXT2_FT_DIR) {
    remove_directory(rm_inode);
  }
  unlink_inode(rm_inodenum);
  return 0;
}
