#pragma once

#include "FDE/Core/Event.hpp"

namespace FDE
{

class FDE_API MouseButtonPressedEvent : public Event
{
  public:
    MouseButtonPressedEvent(int button, float x, float y) : m_button(button), m_x(x), m_y(y) {}
    int GetButton() const { return m_button; }
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }

    FDE_EVENT_TYPE(MouseButtonPressed)
    FDE_EVENT_CATEGORY(EventCategoryMouse | EventCategoryInput)

  private:
    int m_button;
    float m_x;
    float m_y;
};

class FDE_API MouseButtonReleasedEvent : public Event
{
  public:
    MouseButtonReleasedEvent(int button, float x, float y) : m_button(button), m_x(x), m_y(y) {}
    int GetButton() const { return m_button; }
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }

    FDE_EVENT_TYPE(MouseButtonReleased)
    FDE_EVENT_CATEGORY(EventCategoryMouse | EventCategoryInput)

  private:
    int m_button;
    float m_x;
    float m_y;
};

class FDE_API MouseMovedEvent : public Event
{
  public:
    MouseMovedEvent(float x, float y) : m_x(x), m_y(y) {}
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }

    FDE_EVENT_TYPE(MouseMoved)
    FDE_EVENT_CATEGORY(EventCategoryMouse | EventCategoryInput)

  private:
    float m_x;
    float m_y;
};

class FDE_API MouseScrolledEvent : public Event
{
  public:
    MouseScrolledEvent(float offsetX, float offsetY) : m_offsetX(offsetX), m_offsetY(offsetY) {}
    float GetOffsetX() const { return m_offsetX; }
    float GetOffsetY() const { return m_offsetY; }

    FDE_EVENT_TYPE(MouseScrolled)
    FDE_EVENT_CATEGORY(EventCategoryMouse | EventCategoryInput)

  private:
    float m_offsetX;
    float m_offsetY;
};

} // namespace FDE
