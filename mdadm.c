#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mdadm.h"
#include "jbod.h"


uint32_t op_func(jbod_cmd_t command, int disk_num, int block_num){
  return (command  << 12 | disk_num << 0 | block_num << 4);
}

int mounted;

int mdadm_mount(void) {

  if(jbod_operation(op_func(JBOD_MOUNT, 0, 0), NULL) == 0){
    mounted = 1;
    return 1;
    }
  return -1;
}

int mdadm_unmount(void) {

  if(jbod_operation(op_func(JBOD_UNMOUNT, 0, 0), NULL) == 0){
    mounted = 0;
    return 1;
    }
  return -1;
}


int minimum(int x, int y){
  if (x >= y) return y;
  return x;
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf) {
  
  if ((start_addr + read_len) > 1048575) return -1; 
  if (read_len > 1024) return -2;                   
  if (mounted == 0) return -3;
  if (read_buf == NULL && read_len > 0) return -4;
  //if (read_buf == 0 && read_len > 0) return 0;

  int disknum = start_addr / JBOD_DISK_SIZE;
  int blocknum = (start_addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;
  int offset = start_addr % JBOD_BLOCK_SIZE;
  int remaining_len = read_len; //total remaining length to read
  int bytes_read = 0; //bytes that have been read
  uint8_t temp [JBOD_BLOCK_SIZE];

  while (remaining_len > 0){ //while loop so the code continues to run until the total remaining length to read is 0
    uint32_t op = op_func(JBOD_SEEK_TO_DISK, disknum, 0);
    if (jbod_operation(op, NULL) != 0) return -1; //seek operation failed

    op = op_func(JBOD_SEEK_TO_BLOCK, 0, blocknum);
    if (jbod_operation(op, NULL) != 0) return -1; //seek operation failed

    op = op_func(JBOD_READ_BLOCK, 0, 0);
    if (jbod_operation(op, temp) != 0) return -1; //read operation failed

    int bytes_to_read = minimum(JBOD_BLOCK_SIZE - offset, remaining_len); //variable to read until the end of the block

    memcpy(read_buf + bytes_read, temp + offset, bytes_to_read);
    bytes_read += bytes_to_read; //increment the number of bytes read after memcpy
    remaining_len -= bytes_to_read; //decrease the remaining length to read
    offset = 0; //reset the offset everytime it gets to the end of a block

    if (remaining_len != 0){
      blocknum++; //if it gets to the end of the block, blocknum will increase
      if (blocknum >= JBOD_NUM_BLOCKS_PER_DISK){
        disknum++; //if it gets to the end of the disk, disknum will increase
        blocknum = 0; //resets blocknum as it gets to a new disk
      }
    }
  }
  return read_len;
}
