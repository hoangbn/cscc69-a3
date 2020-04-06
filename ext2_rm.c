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
    fprintf(stderr, "Usage: ./ext2_rm <image file path> <absolute file/link path>\n");
    exit(1);
  }
  //load disk and get the arg for sourse and destination
  loaddisk(argv[1]);
  print_disk_image(); // TODO: remove
  char *rm_path = argv[2];
  //check if file to rm exists
  char *rm_file_name;
  int second_last_section_inodenum;
  struct ext2_inode *second_last_dir = path_walk_second_last(rm_path,
                                    &rm_file_name, &second_last_section_inodenum);
  // validate if given destination path was valid
  if (second_last_dir == NULL) {
    printf("No such file or directory\n");
    return ENOENT;
  }
  // if given path is "/", give an error
  if (rm_file_name == NULL) {
    printf("This is a directory. Please specify a file or a link\n");
    return EISDIR;
  }
  // validate given path points to an existent file or link
  int rm_inodenum;
  struct ext2_inode *rm_inode = get_next_inode(second_last_dir, rm_file_name, &rm_inodenum);
  if (rm_inode == NULL) {
    printf("No such file or link to remove\n");
    return ENOENT;
  }
  if (get_type_inode(rm_inode) == EXT2_FT_DIR) {
    printf("This is a directory. Please specify a file or a link\n");
    return EISDIR;
  }
  // remove directory entry for with given name
  remove_dir_entry(second_last_dir, rm_file_name);
  // unlink, free the inode, free it's blocks if it has no more links
  unlink_inode(rm_inodenum);
  print_disk_image(); // TODO: remove
  return 0;
}
