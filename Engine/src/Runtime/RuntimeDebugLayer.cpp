#include "FDE/pch.hpp"
#include "FDE/Runtime/RuntimeDebugLayer.hpp"
#include "FDE/Runtime/RuntimeSession.hpp"
#include "FDE/Scene/Components.hpp"
#include "FDE/Scene/Scene.hpp"
#include "FDE/Scene/World.hpp"
#include "imgui.h"

namespace FDE
{

RuntimeDebugLayer::RuntimeDebugLayer(RuntimeSession* session, float* smoothedFps, bool* debugUiEnabled)
    : Layer("RuntimeDebugLayer")
    , m_session(session)
    , m_smoothedFps(smoothedFps)
    , m_debugUiEnabled(debugUiEnabled)
{
}

void RuntimeDebugLayer::OnUpdate()
{
    if (!m_debugUiEnabled || !*m_debugUiEnabled || !m_session || !m_smoothedFps)
        return;

    ImGui::SetNextWindowBgAlpha(0.92f);
    if (ImGui::Begin("[Runtime] Debug", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::Checkbox("Show debug UI (F3)", m_debugUiEnabled);
        ImGui::Separator();
        ImGui::Text("FPS (smoothed): %.1f", static_cast<double>(*m_smoothedFps));

        World* world = m_session->GetWorld();
        Scene* active = world ? world->GetActiveScene() : nullptr;
        if (!active)
        {
            ImGui::TextUnformatted("No active scene.");
        }
        else
        {
            ImGui::Text("Scene: %s", active->GetName().c_str());
            size_t taggedCount = 0;
            for (auto ignored : active->GetRegistry().view<TagComponent>())
            {
                (void)ignored;
                ++taggedCount;
            }
            ImGui::Text("Tagged entities: %zu", taggedCount);

            if (ImGui::TreeNode("Objects (tags)"))
            {
                auto view = active->GetRegistry().view<TagComponent>();
                for (auto e : view)
                {
                    const auto& tag = view.get<TagComponent>(e);
                    ImGui::BulletText("%s", tag.name.c_str());
                }
                ImGui::TreePop();
            }
        }

        ImGui::Separator();
        ImGui::TextUnformatted("RMB drag: look  |  WASD: move  |  Q/E: down/up");
    }
    ImGui::End();
}

} // namespace FDE
