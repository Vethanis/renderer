#ifndef INPUT_H
#define INPUT_H

class Camera;
struct GLFWwindow;

class Input
{
    GLFWwindow* m_glwindow;

    static bool m_rightMouseDown, m_leftMouseDown;
    static double m_scrollOffset, m_relScroll, m_cursorX, m_cursorY, m_relCursorX, m_relCursorY;

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
public:
    Input(GLFWwindow* window);
    void poll();
    void poll(Camera& cam);
    static bool rightMouseDown();
    static bool leftMouseDown();
    static double scrollOffset();
    static double relScroll();
    static double cursorX();
    static double cursorY();
    static double relCursorX();
    static double relCursorY();
    bool getKey(int key);
    const int* begin();
    const int* end();
    static const int* downBegin();
    static const int* downEnd();
    static const int* upBegin();
    static const int* upEnd();
};
#endif
