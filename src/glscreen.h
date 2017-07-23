#ifndef GLSCREEN_H
#define GLSCREEN_H

struct GLScreen{
    unsigned vao, vbo;
    unsigned vertexShader;
    void init();
    void deinit();
    void draw();
};

#endif
