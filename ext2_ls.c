#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include "diskload.h"
#include "ext2.h"
#include "utils.h"

// checks if given name is either . or .. (1 if true 0 if false)
int isCurOrPrev (char* name, int name_len) {
  return ((name_len == 1 && strncmp(".", name, 1) == 0)) || 
            ((name_len == 2 && strncmp("..", name, 2) == 0));
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: ./ext2_ls <image file path> [-a] <absolute path to list>\n");
    exit(1);
  }
  // check for options
  int all_option = 0;
  char *ls_path = argv[2];
  if (strcmp(argv[2], "-a") == 0) {
    all_option = 1;
    ls_path = argv[3];
  }
  // load disk and perform path walk
  loaddisk(argv[1]);
  struct ext2_dir_entry_2 *dir_entry_to_ls;
  struct ext2_inode *inode_to_ls = path_walk(ls_path, &dir_entry_to_ls);
  // check type of inode, and print accordingly
  if (inode_to_ls == NULL) {
    printf("No such file or directory\n");
    return ENOENT;
  } else if (get_type_inode(inode_to_ls) != 'd') {
    printf("%.*s\n", dir_entry_to_ls->name_len, dir_entry_to_ls->name);
  } else {
    // if it's a dir, get the array of blocks, and loop through all entries,
    // in all blocks, printing accordingly
    unsigned int *arr = inode_to_ls->i_block;
    while (*arr != 0) {
      unsigned long pos = (unsigned long)disk + *arr * EXT2_BLOCK_SIZE;
      struct ext2_dir_entry_2 *dir = (struct ext2_dir_entry_2 *)pos;
      do {
        if (all_option || !isCurOrPrev(dir->name, dir->name_len)) {
          printf("%.*s\n", dir->name_len, dir->name);
        }
        pos += dir->rec_len;
        dir = (struct ext2_dir_entry_2 *)pos;
      } while (pos % EXT2_BLOCK_SIZE != 0);
      arr++;
    }
  }
  return 0;
}
