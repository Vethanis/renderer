#ifndef LOADFILE_H
#define LOADFILE_H

#include "ints.h"

char* load_file(const char* path);

void release_file(const char* p);

inline const char* nextline(const char* p){
    for(; p[0] && p[0] != '\n'; p++){};
    if(p[0] == '\n'){
        p++;
    }
    return p;
}

#endif
