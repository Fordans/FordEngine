#pragma once

#include "FDE/Core/Event.hpp"

namespace FDE
{

class FDE_API WindowCloseEvent : public Event
{
  public:
    FDE_EVENT_TYPE(WindowClose)
    FDE_EVENT_CATEGORY(EventCategoryApplication)
};

class FDE_API WindowResizeEvent : public Event
{
  public:
    WindowResizeEvent(int width, int height) : m_width(width), m_height(height) {}

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    FDE_EVENT_TYPE(WindowResize)
    FDE_EVENT_CATEGORY(EventCategoryApplication)

  private:
    int m_width;
    int m_height;
};

class FDE_API WindowFocusEvent : public Event
{
  public:
    FDE_EVENT_TYPE(WindowFocus)
    FDE_EVENT_CATEGORY(EventCategoryApplication)
};

class FDE_API WindowLostFocusEvent : public Event
{
  public:
    FDE_EVENT_TYPE(WindowLostFocus)
    FDE_EVENT_CATEGORY(EventCategoryApplication)
};

} // namespace FDE
