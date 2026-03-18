#include "FDE/pch.hpp"
#include "FDE/ImGui/ImGuiLayer.hpp"
#include "FDE/Core/Event.hpp"
#include "imgui.h"

namespace FDE
{

ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

void ImGuiLayer::OnAttach() {}

void ImGuiLayer::OnDetach() {}

void ImGuiLayer::OnUpdate() {}

void ImGuiLayer::OnEvent(Event& event)
{
    ImGuiIO& io = ImGui::GetIO();
    if (event.IsInCategory(EventCategoryKeyboard) && io.WantCaptureKeyboard)
    {
        event.SetHandled();
        return;
    }
    if (event.IsInCategory(EventCategoryMouse) && io.WantCaptureMouse)
    {
        event.SetHandled();
        return;
    }
}

} // namespace FDE
