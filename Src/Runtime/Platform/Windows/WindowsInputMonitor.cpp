#include "WindowsInputMonitor.h"
#include "Runtime/Platform/PlatformInputMonitor.h"
#include "Runtime/Platform/Windows/WindowsWindow.h"
#include "backends/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>
#include <iostream>


using namespace platform;

void WindowsInputMonitor::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    WindowsWindow* platformWindow = (WindowsWindow*)glfwGetWindowUserPointer(window);
    WindowsInputMonitor* inputMonitor = (WindowsInputMonitor*)platformWindow->GetInputMonitor();

    // std::cout << "[Mouse Scroll] " << xoffset << ", " << yoffset << std::endl;

    auto& callbacks = inputMonitor->m_mouse->GetScrollCallbacks();
    for (auto& callback : callbacks)
    {
        callback(xoffset, yoffset);
    }
}

void WindowsInputMonitor::cursor_callback(GLFWwindow* window, double xpos, double ypos)
{
    WindowsWindow* platformWindow = (WindowsWindow*)glfwGetWindowUserPointer(window);
    WindowsInputMonitor* inputMonitor = (WindowsInputMonitor*)platformWindow->GetInputMonitor();
    glm::vec2 oldPos = inputMonitor->m_mouse->GetCursorPosition();
    glm::vec2 newPos(xpos, ypos);
    inputMonitor->m_mouse->SetCursorPosition(newPos);
    // std::cout << "[Mouse Position] " << oldPos.x << ", " << oldPos.y << " ==> " << xpos << ", " << ypos << std::endl;

    auto& callbacks = inputMonitor->m_mouse->GetCursorPosCallbacks();
    for (auto& callback : callbacks)
    {
        callback(oldPos, newPos);
    }
}

void WindowsInputMonitor::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    WindowsWindow* platformWindow = (WindowsWindow*)glfwGetWindowUserPointer(window);
    WindowsInputMonitor* inputMonitor = (WindowsInputMonitor*)platformWindow->GetInputMonitor();

    Keyboard::Key keyboardKey = Keyboard::Key::MIN;
    std::string keyboardKeyStr;

#define KEYBOARD_CONVERT_CASE(_KEY_)   \
    case GLFW_KEY_##_KEY_:   \
    {   \
        keyboardKey = Keyboard::Key::_KEY_; keyboardKeyStr = #_KEY_; break; \
    }

    switch(key)
    {
    KEYBOARD_CONVERT_CASE(W)
    KEYBOARD_CONVERT_CASE(A)
    KEYBOARD_CONVERT_CASE(S)
    KEYBOARD_CONVERT_CASE(D)
    KEYBOARD_CONVERT_CASE(LEFT_CONTROL)
    KEYBOARD_CONVERT_CASE(RIGHT_CONTROL)
    KEYBOARD_CONVERT_CASE(LEFT_SHIFT)
    KEYBOARD_CONVERT_CASE(RIGHT_SHIFT)
    KEYBOARD_CONVERT_CASE(SPACE)
    KEYBOARD_CONVERT_CASE(TAB)
    KEYBOARD_CONVERT_CASE(F11)
    KEYBOARD_CONVERT_CASE(ESCAPE)
    default:
    {
        return;
    }
    }
#undef KEYBOARD_CONVERT_CASE

    std::vector<Keyboard::callback>* callbacks = nullptr;
    std::string actionStr;
    switch (action)
    {
    case GLFW_REPEAT:
    case GLFW_PRESS:
    {
        callbacks = &inputMonitor->m_keyboard->GetPressedCallbacks(keyboardKey);
        actionStr = "PRESS";
        break;
    }
    case GLFW_RELEASE:
    {
        callbacks = &inputMonitor->m_keyboard->GetUpCallbacks(keyboardKey);
        actionStr = "RELEASE";
        break;
    }
    default:
    {
        return;
    }
    }

    // std::cout << "[Keyboard Monitor] " << keyboardKeyStr << " " << actionStr << std::endl;

    for (auto& callback : *callbacks)
    {
        callback();
    }
}

void WindowsInputMonitor::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    WindowsWindow* platformWindow = (WindowsWindow*)glfwGetWindowUserPointer(window);
    WindowsInputMonitor* inputMonitor = (WindowsInputMonitor*)platformWindow->GetInputMonitor();

    Mouse::Button mouseBtn = Mouse::Button::MIN;
    std::string mouseBtnStr;

#define MOUSE_BTN_CONVERT_CASE(_BTN_)   \
    case GLFW_MOUSE_BUTTON_##_BTN_:   \
    {   \
        mouseBtn = Mouse::Button::_BTN_; mouseBtnStr = #_BTN_; break; \
    }

    switch (button)
    {
    MOUSE_BTN_CONVERT_CASE(LEFT)
    MOUSE_BTN_CONVERT_CASE(RIGHT)
    MOUSE_BTN_CONVERT_CASE(MIDDLE)
    default:
    {
        return;
    }
    }
#undef MOUSE_BTN_CONVERT_CASE
    std::vector<Mouse::callback>* callbacks = nullptr;
    std::string actionStr;
    switch (action)
    {
    case GLFW_REPEAT:
    case GLFW_PRESS:
    {
        callbacks = &inputMonitor->m_mouse->GetPressedCallbacks(mouseBtn);
        actionStr = "PRESS";
        break;
    }
    case GLFW_RELEASE:
    {
        callbacks = &inputMonitor->m_mouse->GetUpCallbacks(mouseBtn);
        actionStr = "RELEASE";
        break;
    }
    default:
    {
        return;
    }
    }

    // std::cout << "[Mouse Monitor] " << mouseBtnStr << " " << actionStr << std::endl;

    for (auto& callback : *callbacks)
    {
        callback();
    }
}


void WindowsInputMonitor::Init(PlatformWindow* window)
{
    WindowsWindow* wwindow = static_cast<WindowsWindow*>(window);
    assert(wwindow);

    GLFWwindow* glfwWindow = (GLFWwindow*)wwindow->GetRawHandler();
    assert(glfwWindow);

    m_mouse.reset(new Mouse());
    m_keyboard.reset(new Keyboard());


    glfwSetMouseButtonCallback(glfwWindow, mouse_button_callback);
    glfwSetKeyCallback(glfwWindow, key_callback);
    glfwSetCursorPosCallback(glfwWindow, cursor_callback);
    glfwSetScrollCallback(glfwWindow, scroll_callback);
}

void WindowsInputMonitor::UnInit(PlatformWindow* window)
{

}