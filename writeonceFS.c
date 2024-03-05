/**
 * File: writeonceFS.c
 * Authors: Vikram Sahai Saxena(vs799), Vishwas Gowdihalli Mahalingappa(vg421)
 * iLab machine tested on: -ilab1.cs.rutgers.edu
 */
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

//File System Size = 4MB
#define FILE_SYSTEM_SIZE 4*1024*1024

//Block Size = 1KB
#define BLOCK_CHUNK_SIZE 1024

//Maximum Number of Disk Blocks = 4K
#define NO_OF_DISK_BLOCKS 4*1024

//Maximum Number of Files
#define NO_OF_FILES 50

//Maximum File Name length
#define MAX_FILENAME_LEN 16

//Maximum Number of File Descriptors
#define MAX_FILE_DESCRIPTORS 15


static int disk_handle; //disk handle for a creaqted disk
static int disk_open = 0; //flag to indicate if disk is open: 0 = closed, 1 = open

//helper method declarations
int ready_disk(char *file_name);
int open_disk(char *file_name);
int close_disk();
int read_block(int block_index, char *buffer);
int write_block(int block_index, char *buffer);
int disk_init(char *file_name, void *mem_address);
char search_file(char* name);
int available_file_des(char file_index);
int search_next_block(int current, char file_index);
int search_available_block(char file_index);
int wo_create(char *file_name);

//enum declarations
typedef enum {NO, YES} in_use;
typedef enum {WO_CREAT = 1} mode;
typedef enum {WO_RDONLY = 2, WO_WRONLY = 3, WO_RDWR = 4} flags;

//super block structure
typedef struct {
    int inode_block_index; //inode block index
    int inode_block_size; //inode block size
    int data_block_index; //data block index
} super_block;

//inode structure
typedef struct {
    char fname[MAX_FILENAME_LEN]; //file name
    int fsize; //file size
    int fhead; //first data block position for file
    int fblock_count; //number of file blocks
    in_use file_in_use; //flag to indicate file usage
} inode;

//file descriptor structure
typedef struct {
    char findex; //file index
    int offset; //offset for reading
    in_use fd_in_use; //flag to indicate file descriptor usage
} file_des;

file_des file_des_table[MAX_FILE_DESCRIPTORS]; //Table of file descriptors
super_block *sb_ptr; //super block pointer
inode *inode_ptr; //inode pointer

/**
 * wo_mount() : Attempt to read in an entire 'diskfile'.
 * 
 * @param file_name : File name holding the entire disk
 * @param mem_address : address to read the entire 'disk' in to
 * @return int : 0 on success, any negative number on error
 */
int wo_mount(char* file_name, void* mem_address) {
    //check for proper filename
    if (NULL == file_name) {
        return -1;
    }
    if (NULL == mem_address) {
        mem_address = (char*)malloc(FILE_SYSTEM_SIZE);
    }

    //build initial structures for accessing the disk.
    if (disk_init(file_name, mem_address)) {
        errno = EACCES;
        return -errno;
    }
    if (open_disk(file_name)) {
        errno = (EEXIST == open_disk(file_name)) ? EEXIST: EACCES;
        return -errno;
    }
    
    //read the super block
    memset(mem_address, 0, BLOCK_CHUNK_SIZE);
    if (0 > read_block(0, mem_address)) {
        errno = EACCES;
        return -errno;
    }
    memcpy(&sb_ptr, mem_address, sizeof(sb_ptr));

    //read the inode block
    memset(mem_address, 0, BLOCK_CHUNK_SIZE);
    if (0 > read_block(sb_ptr->inode_block_index, mem_address)) {
        errno = EACCES;
        return -errno;
    }
    inode_ptr = (inode*)malloc(BLOCK_CHUNK_SIZE);
    memcpy(inode_ptr, mem_address, sizeof(inode)*(sb_ptr->inode_block_size));

    //reset all file descriptors in file descriptor table to not in use
    int i = 0;
    while (MAX_FILE_DESCRIPTORS > i) {
        file_des_table[i].fd_in_use = NO;
        i++;
    }
    return 0;
}

/**
 * wo_unmount() : Attempt to write out an entire 'diskfile'.
 * 
 * @param mem_address : disk address
 * @return int : 0 on success, any negative number on error
 */
int wo_unmount(void* mem_address) {
    if (NULL == mem_address) {
        mem_address = (char*)malloc(FILE_SYSTEM_SIZE);
    }

    //write out the disk contents
    memset(mem_address, 0, BLOCK_CHUNK_SIZE);
    char* block_ptr = (char*)mem_address;
    int i = 0;
    while (NO_OF_FILES > i) {
        if (YES == inode_ptr[i].file_in_use) {
            memcpy(block_ptr, &inode_ptr[i], sizeof(inode_ptr[i]));
            block_ptr += sizeof(inode);
        }
        i++;
    }
    if (0 > write_block(sb_ptr->inode_block_index, mem_address)) {
        errno = EACCES;
        return -errno;
    }
    //reset file descriptors
    i = 0;
    while (MAX_FILE_DESCRIPTORS > i) {
        if (YES == file_des_table[i].fd_in_use) {
            file_des_table[i].findex = -1;
            file_des_table[i].offset = 0;
            file_des_table[i].fd_in_use = NO;
        }
        i++;
    }
    free(inode_ptr);
    free(mem_address);
    close_disk();
    return 0;
}

/**
 * wo_open() : Attempt to open/create file
 * 
 * @param file_name : file name that is opened/created in the File System
 * @param fl : file permission flag
 * @param m : mode flag for file creation
 * @return int : 0 on success, any negative number on error
 */
int wo_open(char* file_name, flags fl, mode m) { 
    if (WO_CREAT != m) {//mode is not set to WO_CREAT
        char f_index = search_file(file_name);
        if (0 <= f_index) {
            if (WO_RDONLY == fl || WO_WRONLY == fl || WO_RDWR == fl) {
                int fd = available_file_des(f_index);
                if (0 > fd) {
                    errno = ENOENT;
                    return -errno;
                }
                return fd;
            } else {
                errno = EACCES;
                return -errno;
            }
        } else {
            errno = ENOENT;
            return -errno;
        }
    } else {//mode is set to WO_CREAT
        //create file if file does not exist in File System
        char file_index = search_file(file_name);
        if (0 <= file_index) {
            errno = EEXIST;
            return -errno;
        } else {
            wo_create(file_name);
            char file_index = search_file(file_name);
            int fd = available_file_des(file_index);
            if (0 > fd) {
                return -1;
            }
            return fd;
        }
    }
}

/**
 * wo_read():  read file bytes to buffer
 * 
 * @param fd : file descriptor
 * @param buffer : memory location to read bytes in to
 * @param bytes : number of bytes to read
 * @return int : bytes read on success, any negative number on error
 */
int wo_read(int fd,  void* buffer, int bytes) {
    //check for valid file descriptor
    if(0 >= bytes || !file_des_table[fd].fd_in_use) {
        errno = ENOENT;
        return -errno;
    }
    char f_index = file_des_table[fd].findex;
    inode* file_ptr = &inode_ptr[f_index];
    char block[BLOCK_CHUNK_SIZE] = "";
    int offset = file_des_table[fd].offset;
    int b_index = file_ptr->fhead;
    int blocks = 0;

    //read current block
    while (BLOCK_CHUNK_SIZE <= offset) {
        b_index = search_next_block(b_index, f_index);
        offset -= BLOCK_CHUNK_SIZE;
        blocks++;
    }
    read_block(b_index, block);
    char *buffer_ptr = buffer;
    int read_bytes = 0;
    int i = offset;
    while (BLOCK_CHUNK_SIZE > i) {
        buffer_ptr[read_bytes++] = block[i];
        if (bytes == read_bytes) {
            file_des_table[fd].offset += read_bytes;
            return read_bytes;
        }
        i++;
    }
    strcpy(block,"");
    blocks++;

    //read next blocks
    while (bytes > read_bytes && (file_ptr->fblock_count) >= blocks) {
        b_index = search_next_block(b_index, f_index);
        strcpy(block,"");
        read_block(b_index, block);
        i = 0;
        while (BLOCK_CHUNK_SIZE > i) {
            buffer_ptr[read_bytes++] = block[i];
            if (bytes == read_bytes) {
                file_des_table[fd].offset += read_bytes;
                return read_bytes;
            }
            i++;
        }
        blocks++;
    }
    file_des_table[fd].offset += read_bytes;
    return read_bytes;
}

/**
 * Check to see if the given filedscriptor is valid or has an entry in the current table of open file descriptors.
 * If not, return with error and set errno.
 * If valid, perform the write operation, write bytes from the location given to the file indicated.
 */

/**
 * wo_write() : write bytes from buffer to file
 * 
 * @param fd : file descriptor
 * @param buffer : memory location to write bytes from
 * @param bytes : number of bytes to write
 * @return int : bytes written on success, any negative number on error
 */
int wo_write(int fd,  void* buffer, int bytes) {
    //check for valid file descriptor
    if(0 >= bytes || !file_des_table[fd].fd_in_use) {
        errno = ENOENT;
        return -errno;
    }
    char f_index = file_des_table[fd].findex;
    inode* file_ptr = &inode_ptr[f_index];
    int size = file_ptr->fsize;
    int offset = file_des_table[fd].offset;
    int b_index = file_ptr->fhead;
    int blocks = 0;

    //current block position
    while (BLOCK_CHUNK_SIZE <= offset) {
        b_index = search_next_block(b_index, f_index);
        offset -= BLOCK_CHUNK_SIZE;
        blocks++;
    }
    char *buffer_ptr = buffer;
    int write_bytes = 0;
    char block[BLOCK_CHUNK_SIZE] = "";
    if (-1 != b_index) {
        //write current block
        read_block(b_index, block);
        int i = offset;
        while (BLOCK_CHUNK_SIZE > i) {
            block[i] = buffer_ptr[write_bytes++];
            if (bytes == write_bytes || strlen(buffer_ptr) == write_bytes) {
                write_block(b_index, block);
                file_des_table[fd].offset += write_bytes;
                if(size < file_des_table[fd].offset){
                    file_ptr->fsize = file_des_table[fd].offset;
                }
                return write_bytes;
            }
            i++;
        }
        write_block(b_index, block);
        blocks++;
    }

    //write next blocks
    strcpy(block, "");
    while ((bytes > write_bytes) && ((strlen(buffer_ptr)) > write_bytes) && ((file_ptr->fblock_count) > blocks)) {
        b_index = search_next_block(b_index, f_index);
        int i = 0;
        while (BLOCK_CHUNK_SIZE > i) {
            block[i] = buffer_ptr[write_bytes++];
            if (bytes == write_bytes || strlen(buffer_ptr) == write_bytes) {
                write_block(b_index, block);
                file_des_table[fd].offset += write_bytes;
                if(size < file_des_table[fd].offset){
                    file_ptr->fsize = file_des_table[fd].offset;
                }
                return write_bytes;
            }
            i++;
        }
        write_block(b_index, block);
        blocks++;
    }

    //write to new blocks
    strcpy(block, "");
    while (bytes > write_bytes && strlen(buffer_ptr) > write_bytes) {
        b_index = search_available_block(f_index);
        file_ptr->fblock_count++;
        if (-1 == file_ptr->fhead) {
            file_ptr->fhead = b_index;
        }
        if (0 > b_index){
            return -1;
        }
        int i =0;
        while (BLOCK_CHUNK_SIZE > i) {
            block[i] = buffer_ptr[write_bytes++];
            if (bytes == write_bytes || strlen(buffer_ptr) == write_bytes) {
                write_block(b_index, block);
                file_des_table[fd].offset += write_bytes;
                if (size < file_des_table[fd].offset){
                    file_ptr->fsize = file_des_table[fd].offset;
                }
                return write_bytes;
            }
            i++;
        }
        write_block(b_index, block);
    }

    file_des_table[fd].offset += write_bytes;
    if(size < file_des_table[fd].offset){
        file_ptr->fsize = file_des_table[fd].offset;
    }
    return write_bytes;
}

/**
 * wo_close() : close file in the File System.
 * 
 * @param fd : file descriptor
 * @return int : 0 on success, any negative number on error
 */
int wo_close(int fd) {
    //Check if the given filedescriptor is valid or has an entry in the current table of open file descriptors
    if(0 > fd || MAX_FILE_DESCRIPTORS <= fd || !file_des_table[fd].fd_in_use) {
        errno = ENOENT;
        return -errno;
    }
    file_des* fds = &file_des_table[fd];
    fds->fd_in_use = NO;
    return 0;
}

/**
 * ready_disk() : ready a disk for open/create from File System.
 * 
 * @param file_name : File System file name
 * @return int : 0 on success, any negative number on error
 */
int ready_disk(char *file_name) { 
  int f;
  char buffer[BLOCK_CHUNK_SIZE];
  if (!file_name) {
    return -1;
  }
  if (0 > (f = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644))) {
    errno = EACCES;
    return -errno;
  }
  memset(buffer, 0, BLOCK_CHUNK_SIZE);
  int i = 0;
  while (NO_OF_DISK_BLOCKS > i) {
    write(f, buffer, BLOCK_CHUNK_SIZE);
    i++;
  }
  close(f);
  return 0;
}

/**
 * open_disk(): open a disk from File System.
 * 
 * @param file_name: File System file name 
 * @return int : 0 on success, any negative number on error
 */
int open_disk(char *file_name) {
  int f;
  if (!file_name) {
    return -1;
  }  
  if (disk_open) {
    errno = EEXIST;
    return -errno;
  }
  if (0 > (f = open(file_name, O_RDWR, 0644))) {
    errno = EACCES;
    return -errno;
  }
  disk_open = 1;
  disk_handle = f;
  return 0;
}

/**
 * close_disk() : close the disk representing the File System 
 * 
 * @return int : 0 on success, any negative number on error
 */
int close_disk() {
  if (!disk_open) {
    return -1;
  }
  close(disk_handle);
  disk_handle = disk_open = 0;
  return 0;
}

/**
 * read_block() : read block contents to buffer
 * 
 * @param block_index : block index
 * @param buffer : buffer
 * @return int : 0 on success, any negative number on error
 */
int read_block(int block_index, char *buffer) {
  if (!disk_open) {
    errno = EACCES;
    return -errno;
  }
  if ((0 > block_index) || (NO_OF_DISK_BLOCKS <= block_index)) {
    return -1;
  }
  if (0 > lseek(disk_handle, block_index*BLOCK_CHUNK_SIZE, SEEK_SET)) {
    return -1;
  }
  if (0 > read(disk_handle, buffer, BLOCK_CHUNK_SIZE)) {
    return -1;
  }
  return 0;
}

/**
 * write_block() : write contents from buffer to block
 * 
 * @param block_index : block index
 * @param buffer : buffer
 * @return int : 0 on success, any negative number on error
 */
int write_block(int block_index, char *buffer) {
  if (!disk_open) {
    errno = EACCES;
    return -errno;
  }
  if ((0 > block_index) || (NO_OF_DISK_BLOCKS <= block_index)) {
    return -1;
  }
  if (0 > lseek(disk_handle, block_index*BLOCK_CHUNK_SIZE, SEEK_SET)) {
    return -1;
  }
  if (0 > write(disk_handle, buffer, BLOCK_CHUNK_SIZE)) {
    return -1;
  }
  return 0;
}

/**
 * disk_init() : initialize structures for accessing disk.
 * 
 * @param file_name : File System file name
 * @param mem_address : address to read the entire 'disk' in to
 * @return int : 0 on success, any negative number on error
 */
int disk_init(char *file_name, void *mem_address) {
    if (ready_disk(file_name)) {
        errno = EACCES;
        return -errno;
    }
    if (open_disk(file_name)) {
        errno = (EEXIST == open_disk(file_name)) ? EEXIST: EACCES;
        return -errno;
    }
    
    //Initialize the structure for super block
    sb_ptr = (super_block*)malloc(sizeof(super_block));
    if (NULL == sb_ptr) {
        return -1;
    }
    sb_ptr->inode_block_index = 1;
    sb_ptr->inode_block_size = 0;
    sb_ptr->data_block_index = 2;
    memset(mem_address, 0, BLOCK_CHUNK_SIZE);
    memcpy(mem_address, &sb_ptr, sizeof(super_block));
    if (0 > write_block(0, mem_address)) {
        errno = EACCES;
        return -errno;
    }
    free(sb_ptr);
    close_disk();
    return 0;
}

/**
 * search_file() : Search for a file in the File System disk
 * 
 * @param file_name : file name to search
 * @return char : file index on success, any negative number on error
 */
char search_file(char* file_name) {
    for (char i = 0; i < NO_OF_FILES; i++) {
        if(inode_ptr[i].file_in_use == YES && (0 == strcmp(inode_ptr[i].fname, file_name))) {
            return i;
        }
    }
    return -1;
}

/**
 * available_file_des() : find available file descriptor from file descriptor table
 * 
 * @param file_index : file index
 * @return int : file descriptor on success, any negative number on error
 */
int available_file_des(char file_index) {
    int i = 0;
    while (MAX_FILE_DESCRIPTORS > i) {
        if (NO == file_des_table[i].fd_in_use) {
            file_des_table[i].fd_in_use = YES;
            file_des_table[i].findex = file_index;
            file_des_table[i].offset = 0;
            return i;
        }
        i++;
    }
    return -1;
}

/**
 * search_next_block() : search next block of a file
 * 
 * @param current_block_index : current block index
 * @param file_index : file undex
 * @return int : next block index on success, any negative number on error
 */
int search_next_block(int current_block_index, char file_index) {
    char buffer[BLOCK_CHUNK_SIZE] = "";
    if (BLOCK_CHUNK_SIZE > current_block_index) {
        if (0 > read_block(sb_ptr->data_block_index, buffer)) {
            errno = EACCES;
            return -errno;
        }
        for (int i = current_block_index + 1; i < BLOCK_CHUNK_SIZE; i++) {
            if ((file_index + 1) == buffer[i]) {
                return i;
            }
        }
    } else {
        if (0 > read_block(sb_ptr->data_block_index + 1, buffer)) {
            errno = EACCES;
            return -errno;
        }
        for (int i = current_block_index - BLOCK_CHUNK_SIZE + 1; i < BLOCK_CHUNK_SIZE; i++) {
            if ((file_index + 1) == buffer[i]){
                return i + BLOCK_CHUNK_SIZE;
            }
        }
    }
    return -1;
}

/**
 * search_available_block() : search available data block for writing file
 * 
 * @param file_index : file index
 * @return int : available data block number on success, any negative number on error
 */
int search_available_block(char file_index) {
    char buffer1[BLOCK_CHUNK_SIZE] = "";
    char buffer2[BLOCK_CHUNK_SIZE] = "";
    if (0 > read_block(sb_ptr->data_block_index, buffer1)) {
        errno = EACCES;
        return -errno;
    }
    if (0 > read_block(sb_ptr->data_block_index + 1, buffer2)) {
        errno = EACCES;
        return -errno;
    }

    for (int i = 4; i < BLOCK_CHUNK_SIZE; i++) {
        if ('\0' == buffer1[i]) {
            buffer1[i] = (char)(file_index + 1);
            if (0 > write_block(sb_ptr->data_block_index, buffer1)) {
                errno = EACCES;
                return -errno;
            }
            return i;
        }
    }
    for (int i = 0; i < BLOCK_CHUNK_SIZE; i++) {
        if ('\0' == buffer2[i]) {
            buffer2[i] = (char)(file_index + 1);
            if (0 > write_block(sb_ptr->data_block_index, buffer2)) {
                errno = EACCES;
                return -errno;
            }
            return i;
        }
    }
    return -1;
}

/**
 * wo_create() : create a file in the File System disk if mode is WO_CREAT
 * 
 * @param file_name : file name to create in the File System
 * @return int : 0 on success, any negative number on error
 */
int wo_create(char *file_name) {
    char file_index = search_file(file_name);
    if (0 > file_index) {
        //create and initialize file
        for (char i = 0; i < NO_OF_FILES; i++) {
            if (NO == inode_ptr[i].file_in_use) {
                sb_ptr->inode_block_size++;
                inode_ptr[i].file_in_use = YES;
                strcpy(inode_ptr[i].fname, file_name);
                inode_ptr[i].fsize = 0;
                inode_ptr[i].fhead = -1;
                inode_ptr[i].fblock_count = 0;
                return 0;
            }
        }
        return -1;
    } else {
        return 0;
    }
}