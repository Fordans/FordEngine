#pragma once

#include "FDE/Core/Event.hpp"

namespace FDE
{

class FDE_API KeyEvent : public Event
{
  public:
    int GetKeyCode() const { return m_keyCode; }

  protected:
    explicit KeyEvent(int keyCode) : m_keyCode(keyCode) {}
    int m_keyCode;
};

class FDE_API KeyPressedEvent : public KeyEvent
{
  public:
    KeyPressedEvent(int keyCode, int repeatCount) : KeyEvent(keyCode), m_repeatCount(repeatCount) {}
    int GetRepeatCount() const { return m_repeatCount; }

    FDE_EVENT_TYPE(KeyPressed)
    FDE_EVENT_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

  private:
    int m_repeatCount;
};

class FDE_API KeyReleasedEvent : public KeyEvent
{
  public:
    explicit KeyReleasedEvent(int keyCode) : KeyEvent(keyCode) {}

    FDE_EVENT_TYPE(KeyReleased)
    FDE_EVENT_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
};

} // namespace FDE
