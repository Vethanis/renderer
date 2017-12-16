#include "window.h"

#include "myglheaders.h"
#include "framecounter.h"
#include <cassert>
#include <cstdio>
#include "profiler.h"

void error_callback(int error, const char* description)
{
    puts(description);
}

Window::Window(int width, int height, int major_ver, int minor_ver, const char* title){
    glfwSetErrorCallback(error_callback);
    assert(glfwInit());
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major_ver);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor_ver);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(width, height, title, NULL, NULL);
    assert(window);
    glfwMakeContextCurrent(window);
    glewExperimental=true;
    glViewport(0, 0, width, height);
    assert(glewInit() == GLEW_OK);
    glGetError();    // invalid enumerant shows up here, just part of glew being itself.
    glfwSwapInterval(0);
}

Window::~Window(){
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::open(){
    return !glfwWindowShouldClose(window);
}
void Window::swap(){
    ProfilerEvent("Window::swap");
    glfwSwapBuffers(window);
    frameCompleted();
}
