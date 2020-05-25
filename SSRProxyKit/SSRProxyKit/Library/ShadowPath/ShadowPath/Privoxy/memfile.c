//
//  memfile.c
//  ShadowPath
//
//  Created by gaoxiaowei on 2019/8/30.
//  Copyright © 2019 TouchingApp. All rights reserved.
//

#include "memfile.h"
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "jcc.h"

static int memfile_fd = -1;
static char*mem_file_path =NULL;
static char *memfile_base_ptr = NULL;
static size_t memfile_default_mamp_size=0;
static size_t memfile_curoffset=0;
static size_t memfile_size=0;

static  pthread_mutex_t memfile_mutex;



bool memfile_zeroFillFile(int fd, size_t startPos, size_t size) {
    if (fd < 0) {
        return false;
    }

    if (lseek(fd, startPos, SEEK_SET) < 0) {
        printf("fail to lseek fd[%d], error:%s", fd, strerror(errno));
        return false;
    }

    static const char zeros[4 * 1024] = {0};
    while (size >= sizeof(zeros)) {
        if (write(fd, zeros, sizeof(zeros)) < 0) {
            printf("fail to write fd[%d], error:%s", fd, strerror(errno));
            return false;
        }
        size -= sizeof(zeros);
    }
    if (size > 0) {
        if (write(fd, zeros, size) < 0) {
            printf("fail to write fd[%d], error:%s", fd, strerror(errno));
            return false;
        }
    }
    pthread_mutex_init(&memfile_mutex, 0);
    return true;
}

bool memfile_ftruncate(size_t size) {
    if (size == memfile_size) {
        return true;
    }
    size_t oldSize = memfile_size;
    memfile_size = size;
    // round up to (n * pagesize)
    if (memfile_size < memfile_default_mamp_size || (memfile_size % memfile_default_mamp_size != 0)) {
        memfile_size = ((memfile_size / memfile_default_mamp_size) + 1) * memfile_default_mamp_size;
    }
    if (ftruncate(memfile_fd, memfile_size) != 0) {
        printf("fail to truncate to size %zu, %s", memfile_size, strerror(errno));
        memfile_size = oldSize;
        return false;
    }
    if (memfile_size > oldSize) {
        memfile_zeroFillFile(memfile_fd, oldSize, memfile_size - oldSize);
    }
    return true;
}

void create_mem_file(const char*path,size_t mem_size){
    if(path ==NULL){
        return;
    }
    
    mem_file_path=(char*)malloc(strlen(path));
    strcpy(mem_file_path, path);
    
    memfile_default_mamp_size = getpagesize();
    memfile_fd = open(path, O_CREAT|O_RDWR | O_CREAT, S_IRWXU);
    if (memfile_fd < 0) {
        printf("fail to open:%@, %s", path, strerror(errno));
    } else {
        memfile_ftruncate(mem_size);
    }
        
    memfile_base_ptr = mmap(NULL, memfile_size, PROT_READ | PROT_WRITE, MAP_SHARED, memfile_fd, memfile_curoffset);
    if (memfile_base_ptr != MAP_FAILED) {
        memset(memfile_base_ptr, 0, memfile_size);
    }
    else{
        printf("fail to mmap %@ with size %zu at position %zu, %s", path, memfile_size, memfile_curoffset, strerror(errno));
    }
    
}

void destroy_mem_file(){
   if(memfile_base_ptr!=NULL){
        munmap(memfile_base_ptr, memfile_size);
        memfile_base_ptr=NULL;
   }
   if (memfile_fd >= 0) {
        close(memfile_fd);
        memfile_fd = -1;
    }
    if(mem_file_path!=NULL){
        free(mem_file_path);
        mem_file_path=NULL;
    }
}

char* mem_file_malloc(size_t size){
    pthread_mutex_lock(&memfile_mutex);
    if(memfile_base_ptr ==NULL){
        return NULL;
    }
    
    char* ptr=NULL;
    if(memfile_curoffset+size<=memfile_size){
        ptr=(char*)memfile_base_ptr+memfile_curoffset;
        memfile_curoffset+=size;
    }
    else{
        printf("fail to allocMemory with size %zu at position %zu, %s", size, memfile_curoffset, strerror(errno));
    }
    pthread_mutex_unlock(&memfile_mutex);
    return ptr;
}



