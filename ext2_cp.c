#include <stdio.h>
#include <stdlib.h>
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
    fprintf(stderr, "Usage: ./ext2_ls <image file path>  <source path> <destination path>\n");
    exit(1);
  }

    //load disk and get the arg for sourse and destination
    loaddisk(argv[1]);
    print_disk_image(); // TODO: remove
    char *source_path = argv[2];
    char *des_path = argv[3];

    //check for valid source file
    FILE *fd = fopen(source_path, "r");
    if (fd == NULL){
        printf("No such file exist\n");
        return ENOENT;
    }
    // get file name and file size
    char *filename = basename(source_path);
    fseek(fd, 0L, SEEK_END);
    size_t fsize = ftell(fd);

    //check if file has already exist in destination
    struct ext2_dir_entry_2 *des_dir;
    struct ext2_inode *des_inode_= path_walk(des_path, &des_dir);
    struct ext2_dir_entry_2 *file_in_dir = get_next_dir_entry(des_inode_, filename);
    if (file_in_dir != NULL){
        exit(EEXIST);
    }
    
    //create inode 
    struct ext2_dir_entry_2 *new_dir_entry = create_dir_entry(
            des_inode_, filename, EXT2_FT_REG_FILE, 0);
    struct ext2_inode *inode = get_inode(new_dir_entry->inode);
    //////////////////
    //perform copy data

    unsigned int bytes_read;
    unsigned char buf[EXT2_BLOCK_SIZE];

    int block_ptr_index = 0;
    
    // Write data to the direct blocks
    while ((block_ptr_index < 12) && ((bytes_read = fread(buf, 1, EXT2_BLOCK_SIZE, fd)) > 0)) {
        
        // Allocate block
        unsigned int block_num = allocate_block();
        inode->i_block[block_ptr_index++] = block_num;
        inode->i_blocks = NUM_DISK_BLKS(inode->i_blocks, EXT2_BLOCK_SIZE);
        
        // Copy data into block
        unsigned char *block = disk + (block_num * EXT2_BLOCK_SIZE);
        memcpy(block, buf, bytes_read);
        
        inode->i_size += bytes_read;
    }
    
    // Return if all data have been written to the file
    if ((bytes_read = fread(buf, 1, EXT2_BLOCK_SIZE, fd)) <= 0) {
        return 0;
    }
    
    // Allocate indirect block and write remaining data
    unsigned int indirect_block_num = allocate_block();
    inode->i_block[block_ptr_index++] = indirect_block_num;
    inode->i_blocks = NUM_DISK_BLKS(inode->i_blocks, EXT2_BLOCK_SIZE);
    
    // Current and starting position in indirect block
    unsigned int *indirect_block = (unsigned int *)
    BLOCK_START(disk, indirect_block_num);
    
    int max_indirect_blocks = EXT2_BLOCK_SIZE / sizeof(unsigned int);
    int num_indirect_blocks = 0;
    
    do {
        // Allocate direct block
        unsigned int direct_block_num = allocate_block();
        *(indirect_block++) = direct_block_num;
        inode->i_blocks = NUM_DISK_BLKS(inode->i_blocks, EXT2_BLOCK_SIZE);
        
        // Copy data into block
        unsigned char *block = BLOCK_START(disk, direct_block_num);
        memcpy(block, buf, bytes_read);
        
        inode->i_size += bytes_read;
        
        num_indirect_blocks++;
        
    } while (num_indirect_blocks < max_indirect_blocks &&
             (bytes_read = fread(buf, 1, EXT2_BLOCK_SIZE, fd)) > 0);
    print_disk_image(); // TODO: remove
}
    

