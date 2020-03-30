#include "diskload.h"
#include "ext2.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: readimg <image file name>\n");
    exit(1);
  }
  printf("%s\n", argv[0]);
  printf("%s\n", argv[1]);
  printf("%s\n", argv[2]);
  if (loaddisk(argv[1]) != 0)
    exit(1);
  struct ext2_inode *dir_to_ls = path_walk(argv[2]);
  print_inode(dir_to_ls);

  return 0;
}
