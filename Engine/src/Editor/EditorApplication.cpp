#include "FDE/Editor/EditorApplication.hpp"
#include "FDE/Editor/EditorConsole.hpp"
#include "FDE/Editor/EditorPreferences.hpp"
#include "FDE/ImGui/ImGuiLayer.hpp"
#include "FDE/Renderer/BufferLayout.hpp"
#include "FDE/Renderer/Renderer.hpp"
#include "FDE/Renderer/VertexBuffer.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include "imgui.h"
#include "FDE/Renderer/Camera2D.hpp"
#include "imgui_impl_opengl3.h"
#include "stb_image.h"
#include <filesystem>
#include <cstdint>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace FDE
{

static std::string GetExecutableDirectory()
{
#if defined(_WIN32)
    char path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) == 0)
        return {};
    std::filesystem::path p(path);
    return p.parent_path().string();
#else
    return {};
#endif
}

static std::string ResolveIconPath(const std::string& baseName)
{
    namespace fs = std::filesystem;
    std::string cwdPath = "Resources/" + baseName;
    if (fs::exists(cwdPath))
        return fs::absolute(cwdPath).string();
    std::string exeDir = GetExecutableDirectory();
    if (!exeDir.empty())
    {
        std::string exePath = exeDir + "/Resources/" + baseName;
        if (fs::exists(exePath))
            return exePath;
    }
    return {};
}

EditorApplication::EditorApplication() : m_preferences(std::make_unique<EditorPreferences>())
{
    if (!Initialize())
    {
        FDE_LOG_CLIENT_ERROR("Failed to initialize EditorApplication");
    }
}

namespace
{

void CreateTriangleMesh(std::shared_ptr<VertexArray>& outVAO)
{
    // Position (vec3) + Color (vec3) per vertex
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,  // left, red
        0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // right, green
        0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f,  // top, blue
    };
    uint32_t indices[] = {0, 1, 2};

    auto vbo = VertexBuffer::Create(vertices, sizeof(vertices));
    BufferLayout layout = {{ShaderDataType::Float3, "a_Position"}, {ShaderDataType::Float3, "a_Color"}};

    outVAO = VertexArray::Create();
    if (outVAO && vbo)
    {
        outVAO->AddVertexBuffer(vbo, layout);
        outVAO->SetIndexBuffer(indices, 3);
    }
}

} // namespace

EditorApplication::~EditorApplication() = default;

void EditorApplication::LoadTitleBarIcon()
{
    if (m_titleBarIconTexture)
        return;

    std::string path = ResolveIconPath("FE.png");
    if (path.empty())
        return;

    int w, h, channels;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!data)
        return;

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    m_titleBarIconTexture = reinterpret_cast<void*>(static_cast<intptr_t>(texId));
    m_titleBarIconWidth = static_cast<float>(w);
    m_titleBarIconHeight = static_cast<float>(h);
}

WindowSpec EditorApplication::GetWindowSpec() const
{
    WindowSpec spec;
    spec.width = m_preferences->GetWindowWidth();
    spec.height = m_preferences->GetWindowHeight();
    spec.title = "Ford Editor";
    spec.decorated = false; // Frameless for unified ImGui style
    return spec;
}

void EditorApplication::OnWindowCreated()
{
    GetLayerStack().PushOverlay(std::make_unique<ImGuiLayer>());
    EditorConsole::Initialize();

    CreateTriangleMesh(m_triangleVAO);

    if (m_preferences->GetMaximized() && GetWindow())
        GetWindow()->Maximize();
}

void EditorApplication::OnRunEnd()
{
    if (Window* w = GetWindow())
        m_preferences->SetMaximized(w->IsMaximized());
}

void EditorApplication::OnUpdate()
{
    RenderMainUI();
    // Title bar drag: exclude right button area
    if (Window* w = GetWindow(); w)
        w->ProcessTitleBarDrag(TITLE_BAR_HEIGHT, TITLE_BAR_BUTTON_AREA_WIDTH);
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("MainWindow", nullptr, window_flags);
    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    if (m_showScene)
        RenderSceneView(dockspace_id);
    if (m_showPreferences)
        RenderPreferencesWindow(dockspace_id);
    if (m_showConsole)
        EditorConsole::Render(dockspace_id);

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
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Scene", nullptr, &m_showScene);
            ImGui::MenuItem("Console", nullptr, &m_showConsole);
            ImGui::MenuItem("Preferences", nullptr, &m_showPreferences);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));

    ImGui::Begin("##TitleBar", nullptr, flags);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

    LoadTitleBarIcon();
    if (m_titleBarIconTexture && m_titleBarIconWidth > 0 && m_titleBarIconHeight > 0)
    {
        float iconSize = TITLE_BAR_HEIGHT - 10.0f;
        ImGui::Image(m_titleBarIconTexture, ImVec2(iconSize, iconSize));
        ImGui::SameLine();
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.0f);
    if (ImFont* titleFont = GetImGuiContext() ? GetImGuiContext()->GetTitleFont() : nullptr)
    {
        ImGui::PushFont(titleFont);
        ImGui::Text("v0.1.0");
        ImGui::PopFont();
    }
    else
    {
        ImGui::Text("v0.1.0");
    }

    ImGui::SameLine(viewport->WorkSize.x - TITLE_BAR_BUTTON_AREA_WIDTH);

    Window* w = GetWindow();
    if (w)
    {
        // 默认无背景，仅悬停/点击时显示；无描边
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

        ImVec4 btnIdle = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        ImVec4 btnHover = ImVec4(0.35f, 0.35f, 0.38f, 0.8f);
        ImVec4 btnActive = ImVec4(0.45f, 0.45f, 0.48f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, btnIdle);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnActive);
        if (ImGui::Button("z", ImVec2(TITLE_BAR_BUTTON_WIDTH, -1)))
            w->Minimize();
        ImGui::SameLine();
        if (w->IsMaximized())
        {
            if (ImGui::Button("O", ImVec2(TITLE_BAR_BUTTON_WIDTH, -1)))
                w->Restore();
        }
        else
        {
            if (ImGui::Button("O", ImVec2(TITLE_BAR_BUTTON_WIDTH, -1)))
                w->Maximize();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, btnIdle);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.42f, 0.38f, 0.40f, 0.9f));  // 金属暗色
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.44f, 0.46f, 1.0f));
        if (ImGui::Button("X", ImVec2(TITLE_BAR_BUTTON_WIDTH, -1)))
            w->RequestClose();
        ImGui::PopStyleColor(3);

        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void EditorApplication::RenderPreferencesWindow(ImGuiID dockspace_id)
{
    if (!m_showPreferences)
        return;

    static bool s_wasOpen = false;
    static int s_width = 1920;
    static int s_height = 1080;
    static bool s_maximized = false;

    if (m_showPreferences && !s_wasOpen)
    {
        s_width = m_preferences->GetWindowWidth();
        s_height = m_preferences->GetWindowHeight();
        s_maximized = m_preferences->GetMaximized();
        s_wasOpen = true;
    }
    if (!m_showPreferences)
        s_wasOpen = false;

    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
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

            if (ImGui::Checkbox("Start maximized", &s_maximized))
                m_preferences->SetMaximized(s_maximized);
            ImGui::SameLine();
            ImGui::TextDisabled("(Takes effect on next launch)");
        }
    }
    ImGui::End();
}

void EditorApplication::RenderSceneView(ImGuiID dockspace_id)
{
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene"))
    {
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        if (viewportSize.x > 0 && viewportSize.y > 0)
        {
            uint32_t width = static_cast<uint32_t>(viewportSize.x);
            uint32_t height = static_cast<uint32_t>(viewportSize.y);

            if (!m_sceneViewport || m_sceneViewport->GetWidth() != width || m_sceneViewport->GetHeight() != height)
            {
                if (!m_sceneViewport)
                    m_sceneViewport = Viewport::Create(width, height);
                else
                    m_sceneViewport->Resize(width, height);
            }

            if (m_sceneViewport->IsValid())
            {
                m_sceneViewport->Bind();

                Renderer::SetClearColor(0.15f, 0.15f, 0.18f, 1.0f);
                Renderer::Clear();

                glEnable(GL_DEPTH_TEST);

                if (m_triangleVAO)
                {
                    glm::mat4 model = glm::mat4(1.0f);
                    glm::mat4 view = glm::mat4(1.0f);
                    glm::mat4 projection = m_sceneCamera.GetViewProjectionMatrix(width, height);
                    Renderer::SetMVP(model, view, projection);
                    Renderer::DrawIndexed(m_triangleVAO);
                }

                glDisable(GL_DEPTH_TEST);

                m_sceneViewport->Unbind();

                ImGui::Image(m_sceneViewport->GetColorAttachmentTextureId(), viewportSize,
                             ImVec2(0, 1), ImVec2(1, 0));

                bool viewportHovered = ImGui::IsItemHovered();
                if (viewportHovered)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
                    {
                        float aspect = static_cast<float>(width) / static_cast<float>(height);
                        float halfWidth = 2.0f / m_sceneCamera.GetZoom();
                        float halfHeight = halfWidth / aspect;
                        float scaleX = (2.0f * halfWidth) / static_cast<float>(width);
                        float scaleY = (2.0f * halfHeight) / static_cast<float>(height);
                        m_sceneCamera.Pan(-io.MouseDelta.x * scaleX, io.MouseDelta.y * scaleY);
                    }
                    float wheel = io.MouseWheel;
                    if (wheel != 0.0f)
                    {
                        ImVec2 mousePos = io.MousePos;
                        ImVec2 itemMin = ImGui::GetItemRectMin();
                        float focusX = mousePos.x - itemMin.x;
                        float focusY = mousePos.y - itemMin.y;
                        m_sceneCamera.ZoomAt(wheel * 0.2f, focusX, focusY, width, height);
                    }
                }
            }
        }
    }
    ImGui::End();
}

bool EditorApplication::Initialize()
{
    return true;
}

} // namespace FDE
