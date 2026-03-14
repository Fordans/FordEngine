#include "FDE/Core/Application.hpp"
#include "FDE/Core/Event.hpp"
#include "FDE/Core/Log.hpp"
#include "FDE/ImGui/ImGuiContext.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace FDE
{

void Application::Run()
{
    m_window = std::make_unique<Window>(GetWindowSpec());
    if (!m_window->IsValid())
    {
        return;
    }

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        FDE_LOG_CLIENT_ERROR("Failed to initialize GLAD");
        return;
    }

    m_imgui = std::make_unique<ImGuiContext>();
    if (!m_imgui->Init(m_window->GetGLFWWindow()))
    {
        FDE_LOG_CLIENT_ERROR("Failed to initialize ImGui");
        return;
    }

    OnWindowCreated();

    while (!m_window->ShouldClose())
    {
        m_window->OnUpdate();
        m_imgui->BeginFrame();

        for (auto& layer : m_layerStack)
        {
            layer->OnUpdate();
        }
        OnUpdate();

        int fb_width, fb_height;
        m_window->GetFramebufferSize(fb_width, fb_height);
        if (fb_width > 0 && fb_height > 0)
        {
            glViewport(0, 0, fb_width, fb_height);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        m_imgui->EndFrame();
        m_window->SwapBuffers();
    }

    OnRunEnd();
    m_imgui->Shutdown();
}

void Application::DispatchEvent(Event& event)
{
    for (auto it = m_layerStack.rbegin(); it != m_layerStack.rend(); ++it)
    {
        if (event.IsHandled())
            break;
        (*it)->OnEvent(event);
    }
}

} // namespace FDE
