#include "FDE/Core/Application.hpp"

namespace FDE
{

void Application::Run()
{
    m_window = std::make_unique<Window>(GetWindowSpec());
    if (!m_window->IsValid())
    {
        return;
    }

    while (!m_window->ShouldClose())
    {
        m_window->OnUpdate();
        OnUpdate();
    }
}

} // namespace FDE
