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
  if (argc < 4) {
    fprintf(stderr, "Usage: ./ext2_cp <image file path> <source path> <absolute destination path>\n");
    exit(1);
  }
  //load disk and get the arg for sourse and destination
  loaddisk(argv[1]);
  print_disk_image(); // TODO: remove
  char *source_path = argv[2];
  char *des_path = argv[3];
  //check for valid source file
  FILE *source_file = fopen(source_path, "r");
  if (source_file == NULL){
    printf("No such file exist\n");
    return ENOENT;
  }
  // get file name and file size
  char *filename = basename(source_path);
  //check if file has already exist in destination
  char* last_section_name;
  int second_last_section_inodenum;
  struct ext2_inode *second_last_dir = path_walk_second_last(des_path,
                                    &last_section_name, &second_last_section_inodenum);
  // validate if given destination path was valid
  if (second_last_dir == NULL) {
    printf("No such file or directory\n");
    return ENOENT;
  }
  char* des_file_name;
  struct ext2_inode *des_dir = second_last_dir;
  // if given path is "/", we want to copy the file to root dir with same file name
  if (last_section_name == NULL) des_file_name = filename;
  // validate the last_section_name: it could either be a directory or empty
  // if empty: we want to cp file to second_last_dir with new file name last_section_name
  // if it's a directory: we want to copy to that directory with same file name
  else {
    struct ext2_inode *last_inode = get_next_inode(second_last_dir, last_section_name, NULL);
    if (last_inode == NULL) des_file_name = last_section_name;
    else if (get_type_inode(last_inode) == EXT2_FT_DIR) {
      des_dir = last_inode;
      des_file_name = filename;
    } else {
      printf("File with that name already exists\n");
      exit(EEXIST);
    }
  }
  // create a dir entry and inode for new (copied) file
  struct ext2_dir_entry_2 *new_dir_entry = create_dir_entry(
          des_dir, des_file_name, EXT2_FT_REG_FILE, 0);
  struct ext2_inode *new_inode = get_inode(new_dir_entry->inode);
  // read from source file and write it to newly allocated inode
  unsigned int bytes_read;
  unsigned char buf[EXT2_BLOCK_SIZE];
  // allocate up to 12 direct blocks and copy data into those blocks
  int block_ptr_index = 0;
  while (block_ptr_index < 12 && 
        (bytes_read = fread(buf, 1, EXT2_BLOCK_SIZE, source_file)) > 0) {
      // allocate block, update inode block information
      unsigned int blocknum = allocate_block();
      new_inode->i_block[block_ptr_index++] = blocknum;
      new_inode->i_blocks = calculate_iblocks(new_inode->i_blocks, EXT2_BLOCK_SIZE);
      // copy data into block, update size of file and modification time
      unsigned char *block = get_block(blocknum);
      memcpy(block, buf, bytes_read);
      new_inode->i_size += bytes_read;
      unsigned int now = (unsigned int) time(NULL);
      new_inode->i_mtime = now;
  }
  // if we read all data, return
  if ((bytes_read = fread(buf, 1, EXT2_BLOCK_SIZE, source_file)) <= 0) {
    print_disk_image(); // TODO: remove
    return 0;
  }
  // else, need to allocate a single indirect block for more data
  // Note: by assignment description, we asssume files need only a single indirection
  unsigned int blocknum_indirect = allocate_block();
  new_inode->i_block[block_ptr_index++] = blocknum_indirect;
  new_inode->i_blocks = calculate_iblocks(new_inode->i_blocks, EXT2_BLOCK_SIZE);
  // create an array of unsinged int, storing block numbers
  unsigned int *indirect_block = (unsigned int *) get_block(blocknum_indirect);
  int max_indirect_blocks = EXT2_BLOCK_SIZE / sizeof(unsigned int);
  int num_indirect_blocks = 0;
  do {
      // allocate new block to store data
      unsigned int blocknum = allocate_block();
      *indirect_block = blocknum;
      new_inode->i_blocks = calculate_iblocks(new_inode->i_blocks, EXT2_BLOCK_SIZE);
      // copy data into block, update size of file and modification time
      unsigned char *block = get_block(blocknum);
      memcpy(block, buf, bytes_read);
      new_inode->i_size += bytes_read;
      unsigned int now = (unsigned int) time(NULL);
      new_inode->i_mtime = now;
      // update number of blocks, indirect_block pointer
      num_indirect_blocks++;
      indirect_block++;
  } while (num_indirect_blocks < max_indirect_blocks &&
            (bytes_read = fread(buf, 1, EXT2_BLOCK_SIZE, source_file)) > 0);

  print_disk_image(); // TODO: remove
  return 0;
}

