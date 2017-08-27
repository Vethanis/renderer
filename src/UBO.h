#ifndef UBO_H
#define UBO_H

class UBO{
    unsigned id;
public:
    void init(void* ptr, unsigned size, const char* name, unsigned* programs, int num_progs);
    void deinit();
    void upload(void* ptr, unsigned size);
};

#endif
