#pragma once 

inline unsigned hash(const char* name){
    const unsigned char* data = (const unsigned char*)name;
    unsigned val = 3759247821;
    while(*data){
        val ^= *data++;
        val *= 0x01000193;
    }
    return val;
}

inline unsigned hash(const void* p, const unsigned len){
    const unsigned char* data = (const unsigned char*)p;
    unsigned val = 3759247821;
    for(unsigned i = 0; i < len; i++){
        val ^= data[i];
        val *= 0x01000193;
    }
    return val;
}