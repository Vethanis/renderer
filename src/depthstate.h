#pragma once

void pushDepthFunc(int mode);
void popDepthFunc();
void pushDepthMask(int mode);
void popDepthMask();
void pushColorMask(int state);
void popColorMask();

struct DepthContext {
    DepthContext(int mode){
        pushDepthFunc(mode);
    }
    ~DepthContext(){
        popDepthFunc();
    }
};

struct DepthMaskContext {
    DepthMaskContext(int mode){
        pushDepthMask(mode);
    }
    ~DepthMaskContext(){
        popDepthMask();
    }
};

struct ColorMaskContext {
    ColorMaskContext(int state){
        pushColorMask(state);
    }
    ~ColorMaskContext(){
        popColorMask();
    }
};

struct DrawModeContext {
    DrawModeContext(int depthFunc, int depthMask, int colorMask){
        pushDepthFunc(depthFunc);
        pushDepthMask(depthMask);
        pushColorMask(colorMask);
    }
    ~DrawModeContext(){
        popDepthFunc();
        popDepthMask();
        popColorMask();
    }
};

namespace DrawMode {
    void init();
};