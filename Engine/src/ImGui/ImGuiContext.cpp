#include "FDE/ImGui/ImGuiContext.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

namespace FDE
{

bool ImGuiContext::Init(GLFWwindow* window)
{
    if (!window || m_initialized)
    {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // ViewportsEnable can cause issues on some systems - enable if multi-window support needed
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

#if defined(__APPLE__)
    const char* glsl_version = "#version 150";
#else
    const char* glsl_version = "#version 130";
#endif

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
    {
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init(glsl_version))
    {
        ImGui_ImplGlfw_Shutdown();
        return false;
    }

    m_initialized = true;
    return true;
}

void ImGuiContext::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

ImGuiContext::~ImGuiContext()
{
    Shutdown();
}

void ImGuiContext::BeginFrame()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiContext::EndFrame()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data)
    {
        ImGui_ImplOpenGL3_RenderDrawData(draw_data);
    }
}

} // namespace FDE
