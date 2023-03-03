#pragma once

#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <set>
#include <utility>
#include <vector>
#include <set>
namespace platform {

template <typename Callback, int CallbackTypeNum>
struct CallbackContainer
{
    int AddCallback(Callback cb, int callbackType = 0)
    {
        int id;
        std::set<int>& avaliableIds = invalidIds[callbackType];
        if (!avaliableIds.empty())
        {
            id = *avaliableIds.begin();
            avaliableIds.erase(id);
            callbacks[callbackType][id] = cb;
        }
        else
        {
            id = callbacks[callbackType].size();
            callbacks[callbackType].emplace_back(cb);
        }
        return id;
    }

    void EraseCallback(int callbackId, int callbackType = 0)
    {
        if (callbackId >= 0 && callbackId < callbacks[callbackType].size())
        {
            callbacks[callbackType][callbackId] = {};
            invalidIds[callbackType].insert(callbackId);
        }
    }

    std::array<std::vector<Callback>, CallbackTypeNum> callbacks;
    std::array<std::set<int>, CallbackTypeNum> invalidIds;
};


class Keyboard
{
public:
    enum Key
    {
        MIN = 0,

        W,A,S,D,
        LEFT_SHIFT,RIGHT_SHIFT,
        LEFT_CONTROL,RIGHT_CONTROL,
        SPACE,
        TAB,

        F11,
        ESCAPE,
        MAX
    };

    using callback = std::function<void()>;
    using callbackId = int;

public:

    inline callbackId AddTargetKeyPressedCallback(Key key,callback cb) { return m_keyPressedCallbackContainer.AddCallback(cb, key); }
    inline callbackId AddTargetKeyUpCallback(Key key,callback cb) { return m_keyUpCallbackContainer.AddCallback(cb, key); }
    inline void EraseTargetKeyPressedCallback(Key key, callbackId id) { m_keyPressedCallbackContainer.EraseCallback(key, id); }
    inline void EraseTargetKeyUpCallback(Key key, callbackId id) { m_keyUpCallbackContainer.EraseCallback(key, id); }

    inline std::vector<callback>& GetPressedCallbacks(Key key) { return m_keyPressedCallbackContainer.callbacks[key]; }
    inline std::vector<callback>& GetUpCallbacks(Key key) { return m_keyUpCallbackContainer.callbacks[key]; }
private:

    CallbackContainer<callback, Key::MAX> m_keyPressedCallbackContainer;
    CallbackContainer<callback, Key::MAX> m_keyUpCallbackContainer;
};

class Mouse
{
public:
    enum Button
    {
        MIN = 0,

        LEFT,RIGHT,MIDDLE,

        MAX
    };

    using callback = std::function<void()>;
    using callbackId = int;

    using cursorPosCallback = std::function<void(const glm::vec2& oldPos, const glm::vec2& newPos)>;
    using scrollCallback = std::function<void(double, double)>;

public:
    inline callbackId AddTargetButtonPressedCallback(Button key,callback cb) { return m_mousePressedCallbackContainer.AddCallback(cb, key); }
    inline callbackId AddTargetButtonUpCallback(Button key,callback cb) { return m_mouseUpCallbackContainer.AddCallback(cb, key); }
    inline void EraseTargetKeyPressedCallback(Button key, callbackId id) { m_mousePressedCallbackContainer.EraseCallback(key, id); }
    inline void EraseTargetKeyUpCallback(Button key, callbackId id) { m_mouseUpCallbackContainer.EraseCallback(key, id); }

    inline std::vector<callback>& GetPressedCallbacks(Button key) { return m_mousePressedCallbackContainer.callbacks[key]; }
    inline std::vector<callback>& GetUpCallbacks(Button key) { return m_mouseUpCallbackContainer.callbacks[key]; }
    inline std::vector<cursorPosCallback>& GetCursorPosCallbacks() { return m_cursorCallbacks.callbacks[0]; }
    inline std::vector<scrollCallback>& GetScrollCallbacks() { return m_scrollCallbacks.callbacks[0]; }

    inline const glm::vec2& GetCursorPosition() { return m_cursorPosition; }
    inline void SetCursorPosition(const glm::vec2& pos) { m_cursorPosition = pos; }

    inline callbackId AddCursorPositionCallback(cursorPosCallback cb) { return m_cursorCallbacks.AddCallback(cb); }
    inline void EraseCursorPositionCallback(callbackId id) { return m_cursorCallbacks.EraseCallback(id); }

    inline callbackId AddScrollCallback(scrollCallback cb) { return m_scrollCallbacks.AddCallback(cb); }
    inline void EraseScrollCallback(callbackId id) { m_scrollCallbacks.EraseCallback(id); }
private:

    CallbackContainer<callback, Button::MAX> m_mousePressedCallbackContainer;
    CallbackContainer<callback, Button::MAX> m_mouseUpCallbackContainer;

    CallbackContainer<cursorPosCallback, 1> m_cursorCallbacks;
    CallbackContainer<scrollCallback, 1> m_scrollCallbacks;

    glm::vec2 m_cursorPosition;
};

struct Joystick
{

};

class PlatformWindow;
class PlatformInputMonitor
{
protected:
    std::unique_ptr<Keyboard> m_keyboard;
    std::unique_ptr<Mouse> m_mouse;
    std::unique_ptr<Joystick> m_joystick;
public:
    PlatformInputMonitor() = default;
    virtual ~PlatformInputMonitor() = default;

    virtual void Init(PlatformWindow* window) = 0;
    virtual void UnInit(PlatformWindow* window) = 0;

    inline Keyboard::callbackId AddKeyboardPressedCallback(Keyboard::Key key, Keyboard::callback cb) { return m_keyboard->AddTargetKeyPressedCallback(key, cb); }
    inline Keyboard::callbackId AddKeyboardUpCallback(Keyboard::Key key, Keyboard::callback cb) { return m_keyboard->AddTargetKeyUpCallback(key, cb); }
    inline void EraseKeyboardPressedCallback(Keyboard::Key key, Keyboard::callbackId id) { m_keyboard->EraseTargetKeyPressedCallback(key, id); }
    inline void EraseKeyboardUpCallback(Keyboard::Key key, Keyboard::callbackId id) { m_keyboard->EraseTargetKeyUpCallback(key, id); }

    inline Mouse::callbackId AddScrollCallback(Mouse::scrollCallback cb) { return m_mouse->AddScrollCallback(cb); }
    inline void EraseScrollCallback(Mouse::callbackId id) { m_mouse->EraseScrollCallback(id); }

    inline Mouse::callbackId AddMousePressedCallback(Mouse::Button btn, Mouse::callback cb) { return m_mouse->AddTargetButtonPressedCallback(btn, cb); }
    inline Mouse::callbackId AddMouseUpCallback(Mouse::Button btn, Mouse::callback cb) { return m_mouse->AddTargetButtonUpCallback(btn, cb); }
    inline void EraseMousePressedCallback(Mouse::Button btn, Mouse::callbackId id) { m_mouse->EraseTargetKeyPressedCallback(btn, id); }
    inline void EraseMouseUpCallback(Mouse::Button btn, Mouse::callbackId id) { m_mouse->EraseTargetKeyUpCallback(btn, id); }

    inline const glm::vec2& GetCursorPosition() { return m_mouse->GetCursorPosition(); }

    inline Mouse::callbackId AddCursorPositionCallback(Mouse::cursorPosCallback cb) { return m_mouse->AddCursorPositionCallback(cb); }
    inline void EraseCursorPositionCallback(Mouse::callbackId id) { m_mouse->EraseCursorPositionCallback(id); }
};

}