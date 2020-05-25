//
//  memfile.h
//  ShadowPath
//
//  Created by gaoxiaowei on 2019/8/30.
//  Copyright © 2019 TouchingApp. All rights reserved.
//

#ifndef memfile_h
#define memfile_h
#include <stdio.h>

void create_mem_file(const char*path,size_t mem_size);

void destroy_mem_file();

char* mem_file_malloc(size_t size);


#endif /* memfile_h */
