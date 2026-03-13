#include "FDE/ImGui/ImGuiContext.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace FDE
{

static std::string ResolveResourcePath(const std::string& relativePath)
{
    namespace fs = std::filesystem;
    std::string cwdPath = "Resources/" + relativePath;
    if (fs::exists(cwdPath))
        return fs::absolute(cwdPath).string();
#if defined(_WIN32)
    char path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) != 0)
    {
        fs::path exePath(path);
        std::string exeDir = exePath.parent_path().string();
        std::string fullPath = exeDir + "/Resources/" + relativePath;
        if (fs::exists(fullPath))
            return fullPath;
    }
#endif
    return {};
}

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

    // 锐利风格：去除所有圆角
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;

    // 金属感浮雕：边框高光 + 阴影
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;

    // 暗色主题 + 暗紫色装饰色 + 金属浮雕
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.52f, 0.80f);       // 金属高光边（银灰）
    colors[ImGuiCol_BorderShadow] = ImVec4(0.02f, 0.02f, 0.02f, 0.70f); // 浮雕阴影
    colors[ImGuiCol_Separator] = ImVec4(0.45f, 0.45f, 0.47f, 0.60f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.09f, 0.09f, 0.96f);    // 窗口金属底
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.80f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.11f, 0.11f, 0.96f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.20f, 0.20f, 0.75f);       // 金属基底
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.32f, 0.28f, 0.28f, 0.85f); // 悬停反光
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.16f, 0.16f, 0.90f);  // 按下凹陷
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.35f, 0.12f, 0.45f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.75f, 0.45f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.25f, 0.60f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.35f, 0.75f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.40f, 0.18f, 0.50f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.55f, 0.28f, 0.65f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.22f, 0.55f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.45f, 0.20f, 0.55f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.55f, 0.28f, 0.65f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.32f, 0.70f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.55f, 0.28f, 0.65f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.65f, 0.35f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.25f, 0.60f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.55f, 0.28f, 0.65f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.65f, 0.35f, 0.75f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.40f, 0.18f, 0.50f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.55f, 0.28f, 0.65f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.50f, 0.24f, 0.60f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.30f, 0.14f, 0.38f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.45f, 0.20f, 0.55f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.50f, 0.25f, 0.60f, 0.35f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.32f, 0.70f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.43f, 0.43f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.28f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.18f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.45f, 0.43f, 0.43f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.30f, 0.28f, 0.28f, 1.00f);

    // 整体放大控件
    style.ScaleAllSizes(1.35f);

    // 自定义字体：Angel wish.ttf
    std::string fontPath = ResolveResourcePath("Angel wish.ttf");
    if (!fontPath.empty())
    {
        io.Fonts->Clear();
        ImFont* font = io.Fonts->AddFontFromFileTTF(
            fontPath.c_str(), 36.0f, nullptr,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        if (!font)
            io.Fonts->AddFontDefault(); // 加载失败时回退到默认字体
        // 标题栏用大号字体
        m_titleFont = io.Fonts->AddFontFromFileTTF(
            fontPath.c_str(), 48.0f, nullptr,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        if (!m_titleFont)
            m_titleFont = font;
    }

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
