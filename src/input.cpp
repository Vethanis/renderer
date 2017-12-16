#include "input.h"

#include "myglheaders.h"
#include "camera.h"
#include "glm/glm.hpp"
#include "array.h"
#include "framecounter.h"

bool Input::m_rightMouseDown = false;
bool Input::m_leftMouseDown = false;
double Input::m_scrollOffset = 0.0;
double Input::m_relScroll = 0.0;
double Input::m_cursorX = 0.0;
double Input::m_cursorY = 0.0;
double Input::m_relCursorX = 0.0;
double Input::m_relCursorY = 0.0;

Array<int, 1024> g_activeKeys;
Array<int, 1024> g_downKeys;
Array<int, 1024> g_upKeys;

Input::Input(GLFWwindow* window) : m_glwindow(window){
    glfwSetInputMode(m_glwindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(m_glwindow, key_callback);
    glfwSetCursorPosCallback(m_glwindow, cursor_position_callback);
    glfwSetMouseButtonCallback(m_glwindow, mouse_button_callback);
    glfwSetScrollCallback(m_glwindow, scroll_callback);
}

void Input::poll(){
    g_downKeys.clear();
    g_upKeys.clear();
    glfwPollEvents();
}

void Input::poll(Camera& cam){
    poll();
    
    glm::dvec3 v(0.0);
    const double dt = frameSeconds();
    v.z -= glfwGetKey(m_glwindow, GLFW_KEY_W) ? dt : 0.0;
    v.z += glfwGetKey(m_glwindow, GLFW_KEY_S) ? dt : 0.0;
    v.x -= glfwGetKey(m_glwindow, GLFW_KEY_A) ? dt : 0.0;
    v.x += glfwGetKey(m_glwindow, GLFW_KEY_D) ? dt : 0.0;
    v.y += glfwGetKey(m_glwindow, GLFW_KEY_SPACE) ? dt : 0.0;
    v.y -= glfwGetKey(m_glwindow, GLFW_KEY_LEFT_SHIFT) ? dt : 0.0;
    cam.move(v * 2.0);
    cam.yaw(m_relCursorX * dt);
    cam.pitch(m_relCursorY * dt);
	m_relCursorX = 0.0;
	m_relCursorY = 0.0;
    cam.update();
}

bool Input::leftMouseDown(){
    return Input::m_leftMouseDown;
}

bool Input::rightMouseDown(){
    return Input::m_rightMouseDown;
}

double Input::scrollOffset(){
    return Input::m_scrollOffset;
}

double Input::relScroll(){
    return Input::m_relScroll;
}

double Input::cursorX(){
    return Input::m_cursorX;
}

double Input::cursorY(){
    return Input::m_cursorY;
}

double Input::relCursorX(){
    return Input::m_relCursorX;
}

double Input::relCursorY(){
    return Input::m_relCursorY;
}

void Input::mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
    if(action == GLFW_PRESS){
        g_downKeys.grow() = button;
        g_activeKeys.uniquePush(button);
    }
    else if(action == GLFW_RELEASE){
        g_upKeys.grow() = button;
        g_activeKeys.findRemove(button);
    }
    if(action == GLFW_PRESS){
        if(button == GLFW_MOUSE_BUTTON_RIGHT)
            Input::m_rightMouseDown = true;
        else if(button == GLFW_MOUSE_BUTTON_LEFT)
            Input::m_leftMouseDown = true;
    }
    else if(action == GLFW_RELEASE){
        if(button == GLFW_MOUSE_BUTTON_RIGHT)
            Input::m_rightMouseDown = false;
        else if(button == GLFW_MOUSE_BUTTON_LEFT)
            Input::m_leftMouseDown = false;
    }
}

void Input::cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
    m_relCursorX = m_cursorX - xpos;
    m_relCursorY = m_cursorY - ypos;
    m_cursorX = xpos;
    m_cursorY = ypos;
}

void Input::scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    m_relScroll = yoffset - m_scrollOffset;
    m_scrollOffset = yoffset;
}

void Input::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(action == GLFW_PRESS){
        g_downKeys.grow() = key;
        g_activeKeys.uniquePush(key);
    }
    else if(action == GLFW_RELEASE){
        g_upKeys.grow() = key;
        g_activeKeys.findRemove(key);
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

bool Input::getKey(int key){
    return glfwGetKey(m_glwindow, key);
}

const int* Input::begin(){
    return g_activeKeys.begin();
}
const int* Input::end(){
    return g_activeKeys.end();
}
const int* Input::downBegin(){
    return g_downKeys.begin();
}
const int* Input::downEnd(){
    return g_downKeys.end();
}
const int* Input::upBegin(){
    return g_upKeys.begin();
}
const int* Input::upEnd(){
    return g_upKeys.end();
}