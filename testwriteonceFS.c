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
#define BLOCK_CHUNK_SIZE 1024
typedef enum {WO_CREAT = 1} mode;
typedef enum {WO_RDONLY = 2, WO_WRONLY = 3, WO_RDWR = 4} flags;

int main(){

 char *disk_name = "file_system.txt";
 mode CREATE=WO_CREAT;
   flags PERMISSION = WO_RDWR;
   char *test_file = "test.txt";


    if(wo_mount(disk_name,NULL) < 0) {
        fprintf(stderr, "wo_mount()\t error.\n");
    }/*
    if(wo_create("test.txt") < 0) {
        fprintf(stderr, "wo_create()\t error.\n");
    }*/

    int fd1;
    if((fd1 = wo_open(test_file,PERMISSION,1)) < 0) {
        fprintf(stderr, "wo_open()\t error.\n");
    }
    int fd2;
    if((fd2 = wo_open(test_file,PERMISSION,0)) < 0) {
        fprintf(stderr, "wo_open()\t error.\n");
    }
    int i;
    char str1[BLOCK_CHUNK_SIZE * 2];
    for(i = 0; i < BLOCK_CHUNK_SIZE / 2; i++) {
        str1[i] = 'a';
        str1[i + BLOCK_CHUNK_SIZE / 2] = 'b';
        str1[i + BLOCK_CHUNK_SIZE] = 'c';
        str1[i + BLOCK_CHUNK_SIZE * 3 / 2] = 'd';
    }
    char str2[BLOCK_CHUNK_SIZE * 2];
    for(i = 0; i < BLOCK_CHUNK_SIZE / 2; i++) {
        str2[i] = 'e';
        str2[i + BLOCK_CHUNK_SIZE / 2] = 'f';
        str2[i + BLOCK_CHUNK_SIZE] = 'g';
        str2[i + BLOCK_CHUNK_SIZE * 3 / 2] = 'h';
    }
    wo_write(fd1, str1, BLOCK_CHUNK_SIZE * 2);
    wo_write(fd1, str2, BLOCK_CHUNK_SIZE * 2);
    if(wo_close(fd1) < 0) {
        fprintf(stderr, "wo_close()\t error.\n");
    }

    if(wo_close(fd2) < 0) {
        fprintf(stderr, "wo_close()\t error.\n");
    }
 /*
    
    if(wo_unmount(NULL) < 0) {
        fprintf(stderr, "wo_unmount()\t error.\n");
    } else {
        fprintf(stderr,"wo_unmount() successful\n");
    }*/
   // disk_init(disk_name);
 /*  
    if(wo_mount(disk_name,NULL) < 0) {
        fprintf(stderr, "wo_mount()\t error.\n");
    }
    else{
        fprintf(stderr, "wo_mount() successful\t successfull.\n");
    }*/
        /*
    if((fd1 = fs_open("test.txt")) < 0) {
        fprintf(stderr, "fs_open()\t error.\n");
    }*/

    char buf1[BLOCK_CHUNK_SIZE * 4] = "";
    char val1[BLOCK_CHUNK_SIZE * 4];
    for(i = 0; i < BLOCK_CHUNK_SIZE / 2; i++) {
        val1[i] = 'a';
        val1[i + BLOCK_CHUNK_SIZE / 2] = 'b';
        val1[i + BLOCK_CHUNK_SIZE] = 'c';
        val1[i + BLOCK_CHUNK_SIZE * 3 / 2] = 'd';
        val1[i + BLOCK_CHUNK_SIZE * 2] = 'i';
        val1[i + BLOCK_CHUNK_SIZE * 5 / 2] = 'm';
        val1[i + BLOCK_CHUNK_SIZE * 3] = 'n';
        val1[i + BLOCK_CHUNK_SIZE * 7 / 2] = 'o';
    }
    int fd3;
    if((fd3 = wo_open(test_file,PERMISSION,0)) < 0) {
        fprintf(stderr, "wo_open()\t error.\n");
    }
    if(wo_read(fd3, buf1, BLOCK_CHUNK_SIZE * 4) < 0) {
        fprintf(stderr, "wo_read()\t error.\n");
    } else {
        printf("wo_read()\t called successfully.\n");
    }
    
  //  for (i = 0; i < BLOCK_CHUNK_SIZE/4 ; ++i) {
   // if (buf1[i] != val1[i]) {
           // fprintf(stderr, "wo_read()\t  Content [%d]  [%c].\n", i, buf1[i]);
   //        fprintf(stderr, "wo_read()\t error: Content [%d] error: [%c] and [%c].\n", i, buf1[i], val1[i]);
     //   break;
    //    }

  //  }
    
   
   return 0;
}
