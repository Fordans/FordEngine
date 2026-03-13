#include "FDE/Editor/EditorApplication.hpp"
#include "FDE/Editor/EditorPreferences.hpp"
#include "imgui.h"

namespace FDE
{

EditorApplication::EditorApplication() : m_preferences(std::make_unique<EditorPreferences>())
{
    if (!Initialize())
    {
        FDE_LOG_CLIENT_ERROR("Failed to initialize EditorApplication");
    }
}

EditorApplication::~EditorApplication() = default;

WindowSpec EditorApplication::GetWindowSpec() const
{
    WindowSpec spec;
    spec.width = m_preferences->GetWindowWidth();
    spec.height = m_preferences->GetWindowHeight();
    spec.title = "Ford Editor";
    spec.decorated = false; // Frameless for unified ImGui style
    return spec;
}

void EditorApplication::OnUpdate()
{
    RenderMainUI();
    // Title bar drag: exclude right 120px (minimize/maximize/close buttons)
    if (Window* w = GetWindow(); w)
        w->ProcessTitleBarDrag(TITLE_BAR_HEIGHT, 120.0f);
}

void EditorApplication::RenderMainUI()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // 自定义标题栏（置顶先绘制）
    RenderTitleBar();

    // 主内容区：位于标题栏下方
    ImVec2 mainPos = ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + TITLE_BAR_HEIGHT);
    ImVec2 mainSize = ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - TITLE_BAR_HEIGHT);

    ImGui::SetNextWindowPos(mainPos);
    ImGui::SetNextWindowSize(mainSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("MainWindow", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
            {
                if (Window* w = GetWindow())
                    w->RequestClose();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::Separator();
            if (ImGui::MenuItem("Preferences...")) { m_showPreferences = true; }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();

    RenderPreferencesWindow();
}

void EditorApplication::RenderTitleBar()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, TITLE_BAR_HEIGHT));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));

    ImGui::Begin("##TitleBar", nullptr, flags);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

    ImGui::Text("Ford Editor");

    ImGui::SameLine(viewport->WorkSize.x - 120.0f);

    Window* w = GetWindow();
    if (w)
    {
        if (ImGui::Button("_", ImVec2(36, -1)))
            w->Minimize();
        ImGui::SameLine();
        if (w->IsMaximized())
        {
            if (ImGui::Button("[]", ImVec2(36, -1)))
                w->Restore();
        }
        else
        {
            if (ImGui::Button("[]", ImVec2(36, -1)))
                w->Maximize();
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
        if (ImGui::Button("X", ImVec2(36, -1)))
            w->RequestClose();
        ImGui::PopStyleColor(3);
    }

    ImGui::End();
}

void EditorApplication::RenderPreferencesWindow()
{
    if (!m_showPreferences)
        return;

    static bool s_wasOpen = false;
    static int s_width = 1600;
    static int s_height = 900;

    if (m_showPreferences && !s_wasOpen)
    {
        s_width = m_preferences->GetWindowWidth();
        s_height = m_preferences->GetWindowHeight();
        s_wasOpen = true;
    }
    if (!m_showPreferences)
        s_wasOpen = false;

    ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Preferences", &m_showPreferences))
    {
        if (ImGui::CollapsingHeader("Window", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Default Resolution");
            ImGui::SetNextItemWidth(120);
            if (ImGui::InputInt("Width", &s_width, 0, 0))
                s_width = (s_width < 640) ? 640 : (s_width > 7680 ? 7680 : s_width);
            ImGui::SetNextItemWidth(120);
            if (ImGui::InputInt("Height", &s_height, 0, 0))
                s_height = (s_height < 480) ? 480 : (s_height > 4320 ? 4320 : s_height);

            if (ImGui::Button("Apply"))
                m_preferences->SetWindowSize(s_width, s_height);
            ImGui::SameLine();
            ImGui::TextDisabled("(Takes effect on next launch)");
        }
    }
    ImGui::End();
}

bool EditorApplication::Initialize()
{
    return true;
}

} // namespace FDE
