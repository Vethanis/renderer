#include "input.h"

#include "myglheaders.h"
#include "camera.h"
#include "glm/glm.hpp"
#include "array.h"

bool Input::m_rightMouseDown = false;
bool Input::m_leftMouseDown = false;
float Input::m_scrollOffset = 0.0f;
float Input::m_relScroll = 0.0f;
float Input::m_cursorX = 0.0f;
float Input::m_cursorY = 0.0f;
float Input::m_relCursorX = 0.0f;
float Input::m_relCursorY = 0.0f;

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

void Input::poll(float dt, Camera& cam){
    g_downKeys.clear();
    g_upKeys.clear();
    glfwPollEvents();
    glm::vec3 v(0.0f);
    v.z -= glfwGetKey(m_glwindow, GLFW_KEY_W) ? dt : 0.0f;
    v.z += glfwGetKey(m_glwindow, GLFW_KEY_S) ? dt : 0.0f;
    v.x -= glfwGetKey(m_glwindow, GLFW_KEY_A) ? dt : 0.0f;
    v.x += glfwGetKey(m_glwindow, GLFW_KEY_D) ? dt : 0.0f;
    v.y += glfwGetKey(m_glwindow, GLFW_KEY_SPACE) ? dt : 0.0f;
    v.y -= glfwGetKey(m_glwindow, GLFW_KEY_LEFT_SHIFT) ? dt : 0.0f;
    cam.move(v * 2.0f);
    cam.yaw(m_relCursorX * dt);
    cam.pitch(m_relCursorY * dt);
    m_relCursorX = 0.0f;
    m_relCursorY = 0.0f;
    cam.update();
}

bool Input::leftMouseDown(){
    return Input::m_leftMouseDown;
}

bool Input::rightMouseDown(){
    return Input::m_rightMouseDown;
}

float Input::scrollOffset(){
    return Input::m_scrollOffset;
}

float Input::relScroll(){
    return Input::m_relScroll;
}

float Input::cursorX(){
    return Input::m_cursorX;
}

float Input::cursorY(){
    return Input::m_cursorY;
}

float Input::relCursorX(){
    return Input::m_relCursorX;
}

float Input::relCursorY(){
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
    m_relCursorX = m_cursorX - (float)xpos;
    m_relCursorY = m_cursorY - (float)ypos;
    m_cursorX = (float) xpos;
    m_cursorY = (float) ypos;
}

void Input::scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    m_relScroll = (float)yoffset - m_scrollOffset;
    m_scrollOffset = (float)yoffset;
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