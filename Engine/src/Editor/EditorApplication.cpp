#if defined(_WIN32)
#include <windows.h>
#endif

#include "FDE/Editor/EditorApplication.hpp"
#include "FDE/Editor/EditorConsole.hpp"
#include "FDE/Editor/EditorPreferences.hpp"
#include "FDE/ImGui/ImGuiLayer.hpp"
#include "FDE/Renderer/BufferLayout.hpp"
#include "FDE/Renderer/Renderer.hpp"
#include "FDE/Renderer/VertexBuffer.hpp"
#include "FDE/Core/FileSystem.hpp"
#include "FDE/Core/Log.hpp"
#include "FDE/Window/Window.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include "imgui.h"
#include "FDE/Renderer/Camera2D.hpp"
#include "imgui_impl_opengl3.h"
#include "stb_image.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <vector>

#if defined(_WIN32)
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <shobjidl.h>
#include <objbase.h>
#endif

namespace FDE
{

EditorApplication::EditorApplication(const std::string& initialProjectPath)
    : m_preferences(std::make_unique<EditorPreferences>())
    , m_showScene(m_preferences->GetShowScene())
    , m_showConsole(m_preferences->GetShowConsole())
    , m_showContentView(m_preferences->GetShowContent())
    , m_showPreferences(m_preferences->GetShowPreferences())
{
    if (!Initialize())
    {
        FDE_LOG_CLIENT_ERROR("Failed to initialize EditorApplication");
        return;
    }

    if (!initialProjectPath.empty())
    {
        std::string projectRoot = std::filesystem::path(initialProjectPath).parent_path().string();
        if (ProjectDescriptor::IsProjectDirectory(projectRoot))
        {
            ProjectDescriptor desc;
            std::string err;
            if (ProjectDescriptor::LoadFromDirectory(projectRoot, desc, err))
            {
                FileSystem::SetProjectRoot(projectRoot);
                m_projectDescriptor = desc;
                m_preferences->SetLastProjectPath(projectRoot);
                m_contentViewCurrentPath.clear();
                FDE_LOG_CLIENT_INFO("Opened project from file: {} ({})", desc.name, projectRoot);
            }
            else
            {
                FDE_LOG_CLIENT_ERROR("Failed to load project: {}", err);
            }
        }
        else
        {
            FDE_LOG_CLIENT_ERROR("Not a valid project: {}", initialProjectPath);
        }
    }
}

namespace
{

#if defined(_WIN32)
/// Shows native folder picker. Returns selected path or empty string on cancel.
std::string ShowFolderPicker(void* parentWindow, const std::string& defaultPath)
{
    std::string result;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return result;

    IFileOpenDialog* pDialog = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                         reinterpret_cast<void**>(&pDialog));
    if (FAILED(hr) || !pDialog)
    {
        CoUninitialize();
        return result;
    }

    DWORD options;
    pDialog->GetOptions(&options);
    pDialog->SetOptions(options | FOS_PICKFOLDERS);

    if (!defaultPath.empty())
    {
        std::filesystem::path path(defaultPath);
        if (std::filesystem::exists(path))
        {
            wchar_t wpath[MAX_PATH];
            if (MultiByteToWideChar(CP_UTF8, 0, path.string().c_str(), -1, wpath, MAX_PATH) > 0)
            {
                IShellItem* pItem = nullptr;
                if (SUCCEEDED(SHCreateItemFromParsingName(wpath, nullptr, IID_IShellItem,
                                                          reinterpret_cast<void**>(&pItem))))
                {
                    pDialog->SetFolder(pItem);
                    pItem->Release();
                }
            }
        }
    }

    HWND hwnd = parentWindow ? static_cast<HWND>(parentWindow) : nullptr;
    hr = pDialog->Show(hwnd);

    if (SUCCEEDED(hr))
    {
        IShellItem* pItem = nullptr;
        hr = pDialog->GetResult(&pItem);
        if (SUCCEEDED(hr) && pItem)
        {
            wchar_t* pPath = nullptr;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);
            if (SUCCEEDED(hr) && pPath)
            {
                int len = WideCharToMultiByte(CP_UTF8, 0, pPath, -1, nullptr, 0, nullptr, nullptr);
                if (len > 0)
                {
                    result.resize(len - 1);
                    WideCharToMultiByte(CP_UTF8, 0, pPath, -1, result.data(), len, nullptr, nullptr);
                }
                CoTaskMemFree(pPath);
            }
            pItem->Release();
        }
    }

    pDialog->Release();
    CoUninitialize();
    return result;
}
#else
std::string ShowFolderPicker(void*, const std::string&)
{
    return {};
}
#endif

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

    std::string path = FileSystem::ResolveEngineResource("FE.png");
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

void EditorApplication::LoadContentViewIcons()
{
    auto loadIcon = [](const char* name) -> void* {
        std::string path = FileSystem::ResolveEngineResource(name);
        if (path.empty())
            return nullptr;
        int w, h, channels;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
        if (!data)
            return nullptr;
        GLuint texId = 0;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
        return reinterpret_cast<void*>(static_cast<intptr_t>(texId));
    };
    if (!m_contentViewFolderIcon)
        m_contentViewFolderIcon = loadIcon("folder.png");
    if (!m_contentViewFileIcon)
        m_contentViewFileIcon = loadIcon("file.png");
}

void EditorApplication::RenderContentView(ImGuiID dockspace_id)
{
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Content"))
    {
        LoadContentViewIcons();

        if (!FileSystem::HasProject())
        {
            ImGui::TextDisabled("No project open");
            ImGui::End();
            return;
        }

        std::string root = FileSystem::GetProjectRoot();
        std::filesystem::path currentPath(root);
        if (!m_contentViewCurrentPath.empty())
            currentPath = std::filesystem::path(root) / m_contentViewCurrentPath;

        if (!std::filesystem::exists(currentPath) || !std::filesystem::is_directory(currentPath))
        {
            m_contentViewCurrentPath.clear();
            currentPath = root;
        }

        // Breadcrumb / navigation
        if (!m_contentViewCurrentPath.empty())
        {
            std::filesystem::path parent = std::filesystem::path(m_contentViewCurrentPath).parent_path();
            if (ImGui::Button(".."))
                m_contentViewCurrentPath = parent.string();
            ImGui::SameLine();
            ImGui::TextDisabled("%s", m_contentViewCurrentPath.c_str());
            ImGui::Separator();
        }

        int iconSizePref = m_preferences->GetContentIconSize();
        float iconSize = static_cast<float>(iconSizePref < 24 ? 24 : (iconSizePref > 256 ? 256 : iconSizePref));
        float cellWidth = iconSize + 24.0f;
        void* folderIcon = m_contentViewFolderIcon;
        void* fileIcon = m_contentViewFileIcon;
        ImVec2 iconSizeVec(iconSize, iconSize);

        auto isHiddenFile = [](const std::string& name) -> bool {
            return name == ".fproject" || name == "FordEditor.cfg" || name == "imgui.ini";
        };

        std::vector<std::string> folders;
        std::vector<std::string> files;

        // Two-phase: collect paths first, then classify. Reduces time holding
        // directory_iterator open and avoids crashes when directory is modified during iteration.
        std::vector<std::string> entryNames;
        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(currentPath))
            {
                try
                {
                    std::string name = entry.path().filename().string();
                    if (!name.empty() && name[0] != '.')
                        entryNames.push_back(std::move(name));
                }
                catch (...)
                {
                    continue;
                }
            }
        }
        catch (...)
        {
            entryNames.clear();
            m_contentViewCurrentPath.clear();
        }

        for (const std::string& name : entryNames)
        {
            try
            {
                std::filesystem::path fullPath = currentPath / name;
                if (!std::filesystem::exists(fullPath))
                    continue;
                if (std::filesystem::is_directory(fullPath))
                    folders.push_back(name);
                else if (std::filesystem::is_regular_file(fullPath))
                {
                    if (!isHiddenFile(name))
                        files.push_back(name);
                }
            }
            catch (...)
            {
                continue;
            }
        }

        std::sort(folders.begin(), folders.end());
        std::sort(files.begin(), files.end());

        ImGui::BeginChild("ContentList", ImVec2(0, 0), false);

        float availWidth = ImGui::GetContentRegionAvail().x;
        int numColumns = availWidth > 0 ? static_cast<int>(availWidth / cellWidth) : 1;
        if (numColumns < 1)
            numColumns = 1;

        auto drawItem = [&](const std::string& name, void* icon, bool isFolder) {
            ImGui::BeginGroup();
            float cursorX = ImGui::GetCursorPosX();
            if (icon)
            {
                ImGui::SetCursorPosX(cursorX + (cellWidth - iconSize) * 0.5f);
                ImGui::Image(icon, iconSizeVec);
            }
            else
            {
                ImGui::SetCursorPosX(cursorX + (cellWidth - 24.0f) * 0.5f);
                ImGui::Text(isFolder ? "[D]" : "[F]");
            }
            ImGui::SetCursorPosX(cursorX);
            ImGui::PushTextWrapPos(cursorX + cellWidth);
            ImGui::TextUnformatted(name.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndGroup();

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && isFolder)
            {
                m_contentViewCurrentPath = m_contentViewCurrentPath.empty()
                                               ? name
                                               : (m_contentViewCurrentPath + "/" + name);
            }
        };

        int col = 0;
        for (const auto& name : folders)
        {
            if (col > 0)
                ImGui::SameLine(col * cellWidth);
            drawItem(name, folderIcon, true);
            col = (col + 1) % numColumns;
        }
        for (const auto& name : files)
        {
            if (col > 0)
                ImGui::SameLine(col * cellWidth);
            drawItem(name, fileIcon, false);
            col = (col + 1) % numColumns;
        }

        ImGui::EndChild();
    }
    ImGui::End();
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
    m_preferences->SetShowScene(m_showScene);
    m_preferences->SetShowContent(m_showContentView);
    m_preferences->SetShowConsole(m_showConsole);
    m_preferences->SetShowPreferences(m_showPreferences);
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
    if (m_showContentView)
        RenderContentView(dockspace_id);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Project"))
                OnNewProject();
            if (ImGui::MenuItem("Open Project"))
                OnOpenProject();
            if (ImGui::MenuItem("Save Project", nullptr, false, FileSystem::HasProject()))
                OnSaveProject();
#if defined(_WIN32)
            ImGui::Separator();
            if (ImGui::MenuItem("Register .fproject association"))
                OnRegisterFileAssociation();
#endif
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
            ImGui::MenuItem("Content", nullptr, &m_showContentView);
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

void EditorApplication::OnNewProject()
{
    void* parentHwnd = nullptr;
#if defined(_WIN32)
    if (Window* w = GetWindow(); w && w->GetGLFWWindow())
    {
        parentHwnd = glfwGetWin32Window(w->GetGLFWWindow());
    }
#endif
    std::string defaultPath = m_preferences->GetLastProjectPath();
    if (defaultPath.empty())
        defaultPath = std::filesystem::current_path().string();
    std::string parentDir = std::filesystem::path(defaultPath).parent_path().string();

    std::string selected = ShowFolderPicker(parentHwnd, parentDir);
    if (selected.empty())
        return;

    std::string projectRoot = selected;
    if (ProjectDescriptor::IsProjectDirectory(projectRoot))
    {
        FDE_LOG_CLIENT_WARN("Directory already contains a project: {}", projectRoot);
        return;
    }

    std::string name = std::filesystem::path(projectRoot).filename().string();
    if (name.empty())
        name = "NewProject";

    ProjectDescriptor desc;
    desc.name = name;
    desc.version = "1.0.0";
    desc.schemaVersion = 1;

    std::string err;
    if (!desc.SaveToDirectory(projectRoot, err))
    {
        FDE_LOG_CLIENT_ERROR("Failed to create project: {}", err);
        return;
    }

    FileSystem::SetProjectRoot(projectRoot);
    m_projectDescriptor = desc;
    m_preferences->SetLastProjectPath(projectRoot);
    m_contentViewCurrentPath.clear();
    FDE_LOG_CLIENT_INFO("Created project: {}", projectRoot);
}

void EditorApplication::OnOpenProject()
{
    void* parentHwnd = nullptr;
#if defined(_WIN32)
    if (Window* w = GetWindow(); w && w->GetGLFWWindow())
    {
        parentHwnd = glfwGetWin32Window(w->GetGLFWWindow());
    }
#endif
    std::string defaultPath = m_preferences->GetLastProjectPath();
    if (defaultPath.empty())
        defaultPath = std::filesystem::current_path().string();

    std::string selected = ShowFolderPicker(parentHwnd, defaultPath);
    if (selected.empty())
        return;

    if (!ProjectDescriptor::IsProjectDirectory(selected))
    {
        FDE_LOG_CLIENT_ERROR("Not a valid project directory (missing .fproject): {}", selected);
        return;
    }

    ProjectDescriptor desc;
    std::string err;
    if (!ProjectDescriptor::LoadFromDirectory(selected, desc, err))
    {
        FDE_LOG_CLIENT_ERROR("Failed to load project: {}", err);
        return;
    }

    FileSystem::SetProjectRoot(selected);
    m_projectDescriptor = desc;
    m_preferences->SetLastProjectPath(selected);
    m_contentViewCurrentPath.clear();
    FDE_LOG_CLIENT_INFO("Opened project: {} ({})", desc.name, selected);
}

void EditorApplication::OnSaveProject()
{
    if (!FileSystem::HasProject() || !m_projectDescriptor)
        return;

    std::string root = FileSystem::GetProjectRoot();
    std::string err;
    if (!m_projectDescriptor->SaveToDirectory(root, err))
    {
        FDE_LOG_CLIENT_ERROR("Failed to save project: {}", err);
        return;
    }
    FDE_LOG_CLIENT_INFO("Saved project: {}", root);
}

#if defined(_WIN32)
namespace
{
bool RegisterFprojectAssociation()
{
    std::string exeDir = FileSystem::GetExecutableDirectory();
    if (exeDir.empty())
        return false;

    std::string exePath = exeDir + "\\FordEditor.exe";
    std::string iconPath = exeDir + "\\Resources\\FE.ico";

    if (!std::filesystem::exists(exePath))
        return false;

    HKEY hKeyExt = nullptr;
    HKEY hKeyProgId = nullptr;
    HKEY hKeyIcon = nullptr;
    HKEY hKeyCmd = nullptr;

    const char* progId = "FordEditor.fproject";
    const char* desc = "Ford Editor Project";

    bool ok = false;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\.fproject", 0, nullptr,
                       REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKeyExt, nullptr) == ERROR_SUCCESS)
    {
        RegSetValueExA(hKeyExt, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(progId),
                      static_cast<DWORD>(strlen(progId) + 1));
        RegCloseKey(hKeyExt);

        if (RegCreateKeyExA(HKEY_CURRENT_USER, ("Software\\Classes\\" + std::string(progId)).c_str(),
                           0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKeyProgId,
                           nullptr) == ERROR_SUCCESS)
        {
            RegSetValueExA(hKeyProgId, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(desc),
                          static_cast<DWORD>(strlen(desc) + 1));

            if (RegCreateKeyExA(hKeyProgId, "DefaultIcon", 0, nullptr, REG_OPTION_NON_VOLATILE,
                               KEY_WRITE, nullptr, &hKeyIcon, nullptr) == ERROR_SUCCESS)
            {
                std::string iconVal = iconPath;
                if (!std::filesystem::exists(iconPath))
                    iconVal = exePath + ",0";
                RegSetValueExA(hKeyIcon, nullptr, 0, REG_SZ,
                              reinterpret_cast<const BYTE*>(iconVal.c_str()),
                              static_cast<DWORD>(iconVal.size() + 1));
                RegCloseKey(hKeyIcon);
            }

            if (RegCreateKeyExA(hKeyProgId, "shell\\open\\command", 0, nullptr,
                                REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKeyCmd,
                                nullptr) == ERROR_SUCCESS)
            {
                std::string cmd = "\"" + exePath + "\" \"%1\"";
                RegSetValueExA(hKeyCmd, nullptr, 0, REG_SZ,
                              reinterpret_cast<const BYTE*>(cmd.c_str()),
                              static_cast<DWORD>(cmd.size() + 1));
                RegCloseKey(hKeyCmd);
            }
            RegCloseKey(hKeyProgId);
            ok = true;
        }
    }
    return ok;
}
} // namespace
#endif

void EditorApplication::OnRegisterFileAssociation()
{
#if defined(_WIN32)
    if (RegisterFprojectAssociation())
        FDE_LOG_CLIENT_INFO("Registered .fproject association. You can now double-click .fproject files to open them.");
    else
        FDE_LOG_CLIENT_ERROR("Failed to register .fproject association.");
#else
    (void)this;
#endif
}

void EditorApplication::RenderPreferencesWindow(ImGuiID dockspace_id)
{
    if (!m_showPreferences)
        return;

    static bool s_wasOpen = false;
    static int s_width = 1920;
    static int s_height = 1080;
    static bool s_maximized = false;
    static int s_contentIconSize = 48;

    if (m_showPreferences && !s_wasOpen)
    {
        s_width = m_preferences->GetWindowWidth();
        s_height = m_preferences->GetWindowHeight();
        s_maximized = m_preferences->GetMaximized();
        s_contentIconSize = m_preferences->GetContentIconSize();
        s_wasOpen = true;
    }
    if (!m_showPreferences)
        s_wasOpen = false;

    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Preferences", &m_showPreferences))
    {
        if (ImGui::CollapsingHeader("Content", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Content View icon size");
            ImGui::SetNextItemWidth(120);
            if (ImGui::SliderInt("##ContentIconSize", &s_contentIconSize, 24, 256, "%d px"))
            {
                s_contentIconSize = (s_contentIconSize < 24) ? 24 : (s_contentIconSize > 256 ? 256 : s_contentIconSize);
                m_preferences->SetContentIconSize(s_contentIconSize);
            }
        }
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
