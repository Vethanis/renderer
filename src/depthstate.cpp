#include "depthstate.h"
#include "myglheaders.h"
#include "array.h"

Array<int, 64> funcStack;
Array<int, 64> maskStack;
Array<int, 64> colorMaskStack;

void updateDepthFunc(){
    glDepthFunc(funcStack.back()); DebugGL();
}

void updateDepthMask(){
    glDepthMask(maskStack.back()); DebugGL();
}

void updateColorMask(){
    int state = colorMaskStack.back();
    glColorMask(state, state, state, state); DebugGL();
}

void pushDepthFunc(int mode){
    funcStack.grow() = mode;
    updateDepthFunc();
}

void popDepthFunc(){
    funcStack.pop();
    updateDepthFunc();
}

void pushDepthMask(int mode){
    maskStack.grow() = mode;
    updateDepthMask();
}

void popDepthMask(){
    maskStack.pop();
    updateDepthMask();
}

void pushColorMask(int state){
    colorMaskStack.grow() = state;
    updateColorMask();
}

void popColorMask(){
    colorMaskStack.pop();
    updateColorMask();
}

void DrawMode::init(){
    pushDepthFunc(GL_LESS);
    pushDepthMask(GL_TRUE);
    pushColorMask(1);
}