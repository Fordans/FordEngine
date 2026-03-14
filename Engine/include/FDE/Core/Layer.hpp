#pragma once

#include "FDE/Core/Event.hpp"
#include "FDE/Export.hpp"
#include <string>

namespace FDE
{

class FDE_API Layer
{
  public:
    explicit Layer(std::string name = "Layer");
    virtual ~Layer() = default;

    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnUpdate() {}
    virtual void OnEvent(Event& event) { (void)event; }

    const std::string& GetName() const { return m_name; }

  protected:
    std::string m_name;
};

} // namespace FDE
