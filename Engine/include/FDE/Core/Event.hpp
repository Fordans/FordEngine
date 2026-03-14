#pragma once

#include "FDE/Export.hpp"

namespace FDE
{

enum class EventType
{
    None = 0,
    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    KeyPressed,
    KeyReleased,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,
    // 扩展预留
};

enum EventCategory
{
    None = 0,
    EventCategoryApplication = 1 << 0,
    EventCategoryInput = 1 << 1,
    EventCategoryKeyboard = 1 << 2,
    EventCategoryMouse = 1 << 3,
};

#define FDE_EVENT_TYPE(type) static EventType GetStaticType() { return EventType::type; } \
    EventType GetEventType() const override { return GetStaticType(); } \
    const char* GetName() const override { return #type; }

#define FDE_EVENT_CATEGORY(category) int GetCategoryFlags() const override { return category; }

class FDE_API Event
{
  public:
    virtual ~Event() = default;

    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
    virtual int GetCategoryFlags() const = 0;

    bool IsInCategory(EventCategory category) const
    {
        return GetCategoryFlags() & category;
    }

    bool IsHandled() const { return m_handled; }
    void SetHandled(bool handled = true) { m_handled = handled; }

  protected:
    bool m_handled = false;
};

} // namespace FDE
