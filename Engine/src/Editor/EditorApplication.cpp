#include "FDE/pch.hpp"
#include "FDE/Editor/EditorApplication.hpp"
#include "FDE/Editor/EditorConsole.hpp"
#include "FDE/Editor/EditorPreferences.hpp"
#include "FDE/ImGui/ImGuiLayer.hpp"
#include "FDE/Renderer/Renderer.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include "FDE/Runtime/RuntimeMeshResolve.hpp"
#include "FDE/Scene/Components.hpp"
#include "FDE/Scene/Object.hpp"
#include "FDE/Scene/Scene3D.hpp"
#include "FDE/Asset/AssetDatabase.hpp"
#include "FDE/Asset/AssetManager.hpp"
#include "FDE/Core/FileSystem.hpp"
#include "FDE/Core/Input.hpp"
#include "FDE/Core/Log.hpp"
#include "FDE/Window/Window.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "imgui.h"
#include "FDE/Renderer/Camera2D.hpp"
#include "FDE/Renderer/Shader.hpp"
#include "imgui_impl_opengl3.h"
#include "stb_image.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <vector>
#include <variant>

#if !defined(_WIN32)
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <shobjidl.h>
#include <objbase.h>
#endif

namespace
{

bool GlfwNavKeyDown(FDE::Window* window, int glfwKey)
{
    if (!window)
        return false;
    GLFWwindow* w = window->GetGLFWWindow();
    if (!w)
        return false;
    return glfwGetKey(w, glfwKey) == GLFW_PRESS;
}

#if defined(_WIN32)
// ClipCursor uses screen space; ImGui rects are GLFW client coordinates — convert with ClientToScreen.
void Win32ClipCursorToSceneRect(GLFWwindow* gw, float minX, float minY, float maxX, float maxY)
{
    if (!gw)
    {
        ::ClipCursor(nullptr);
        return;
    }
    HWND hwnd = glfwGetWin32Window(gw);
    if (!hwnd)
    {
        ::ClipCursor(nullptr);
        return;
    }
    POINT tl{static_cast<LONG>(std::floor(minX)), static_cast<LONG>(std::floor(minY))};
    POINT br{static_cast<LONG>(std::ceil(maxX)), static_cast<LONG>(std::ceil(maxY))};
    if (!::ClientToScreen(hwnd, &tl) || !::ClientToScreen(hwnd, &br))
    {
        ::ClipCursor(nullptr);
        return;
    }
    RECT r;
    r.left = tl.x;
    r.top = tl.y;
    r.right = br.x;
    r.bottom = br.y;
    if (r.right > r.left && r.bottom > r.top)
        ::ClipCursor(&r);
    else
        ::ClipCursor(nullptr);
}

void Win32ReleaseClipCursor()
{
    ::ClipCursor(nullptr);
}
#endif

GLuint s_editorGrid3DVao = 0;
GLuint s_editorGrid3DVbo = 0;

/// XZ ground grid (y=0), same RGBA as ImGui 2D overlay: (128,128,140,200).
void DrawEditorSceneGrid3D(const FDE::Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight,
                           float cellSize)
{
    if (viewportWidth == 0 || viewportHeight == 0)
        return;
    if (cellSize <= 0.001f)
        cellSize = 0.5f;

    const glm::vec3 eye = camera.GetPosition();
    const float distXZ = std::sqrt(eye.x * eye.x + eye.z * eye.z);
    float halfSpan = std::max(20.f, distXZ * 1.25f + 30.f);
    halfSpan = std::min(halfSpan, 400.f);

    constexpr float kMaxCellsPerAxis = 512.f;
    const float maxWorldSpan = kMaxCellsPerAxis * cellSize;
    const float span = std::min(halfSpan * 2.f, maxWorldSpan);

    const float xCenter = eye.x;
    const float zCenter = eye.z;
    const float x0 = std::floor((xCenter - span * 0.5f) / cellSize) * cellSize;
    const float z0 = std::floor((zCenter - span * 0.5f) / cellSize) * cellSize;
    const float x1 = x0 + span;
    const float z1 = z0 + span;

    std::vector<float> verts;
    verts.reserve(static_cast<size_t>(kMaxCellsPerAxis) * 12u * 2u);
    for (float z = z0; z <= z1 + 1e-4f; z += cellSize)
    {
        verts.push_back(x0);
        verts.push_back(0.f);
        verts.push_back(z);
        verts.push_back(x1);
        verts.push_back(0.f);
        verts.push_back(z);
    }
    for (float x = x0; x <= x1 + 1e-4f; x += cellSize)
    {
        verts.push_back(x);
        verts.push_back(0.f);
        verts.push_back(z0);
        verts.push_back(x);
        verts.push_back(0.f);
        verts.push_back(z1);
    }

    if (verts.empty())
        return;

    if (s_editorGrid3DVao == 0)
    {
        glGenVertexArrays(1, &s_editorGrid3DVao);
        glGenBuffers(1, &s_editorGrid3DVbo);
    }
    glBindVertexArray(s_editorGrid3DVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_editorGrid3DVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(verts.size() * sizeof(float)), verts.data(),
                 GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, nullptr);

    FDE::Shader* simple = FDE::Renderer::GetSimpleShader();
    if (!simple)
    {
        glBindVertexArray(0);
        return;
    }

    FDE::Renderer::SetShader(simple);
    simple->Bind();
    simple->SetVec4("u_Color", glm::vec4(128.f / 255.f, 128.f / 255.f, 140.f / 255.f, 200.f / 255.f));

    const glm::mat4 model(1.f);
    const glm::mat4 view = camera.GetViewMatrix();
    const glm::mat4 proj = camera.GetProjectionMatrix(viewportWidth, viewportHeight);
    FDE::Renderer::SetMVP(model, view, proj);

    const GLboolean blendWas = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const GLuint lineVertCount = static_cast<GLuint>(verts.size() / 3u);
    FDE::Renderer::DrawLines(lineVertCount);

    if (!blendWas)
        glDisable(GL_BLEND);

    glBindVertexArray(0);
    FDE::Renderer::UseDefaultShader();
}

FDE::Scene3D* CreateDefaultScene3D(FDE::World* world)
{
    if (!world)
        return nullptr;
    FDE::Scene3D* scene = world->CreateScene3D("Default");
    if (!scene)
        return nullptr;
    world->SetActiveScene(scene);
    FDE::Object sunObj = scene->CreateObject();
    scene->AddComponent<FDE::TagComponent>(sunObj, "Directional Light");
    scene->AddComponent<FDE::DirectionalLightComponent>(sunObj, FDE::DirectionalLightComponent{});
    scene->AddComponent<FDE::SkyboxComponent>(sunObj, FDE::SkyboxComponent{});

    FDE::Object cubeObj = scene->CreateObject();
    scene->AddComponent<FDE::TagComponent>(cubeObj, "Cube");
    scene->AddComponent<FDE::Transform3DComponent>(cubeObj);
    FDE::Mesh3DComponent meshComp;
    meshComp.vertexArray = nullptr;
    meshComp.meshAsset = "builtin:cube";
    scene->AddComponent<FDE::Mesh3DComponent>(cubeObj, meshComp);

    FDE::Object skullObj = scene->CreateObject();
    scene->AddComponent<FDE::TagComponent>(skullObj, "Skull");
    FDE::Transform3DComponent skullTransform;
    skullTransform.position = glm::vec3(2.5f, 0.0f, 0.0f);
    skullTransform.scale = glm::vec3(0.04f);
    scene->AddComponent<FDE::Transform3DComponent>(skullObj, skullTransform);
    FDE::Mesh3DComponent skullMesh;
    skullMesh.vertexArray = nullptr;
    skullMesh.meshAsset = "engine:skull/skull.3ds";
    scene->AddComponent<FDE::Mesh3DComponent>(skullObj, skullMesh);

    FDE::Object skeletonObj = scene->CreateObject();
    scene->AddComponent<FDE::TagComponent>(skeletonObj, "Skeleton");
    FDE::Transform3DComponent skeletonTransform;
    skeletonTransform.position = glm::vec3(-2.5f, 0.0f, 0.0f);
    skeletonTransform.scale = glm::vec3(0.04f);
    scene->AddComponent<FDE::Transform3DComponent>(skeletonObj, skeletonTransform);
    FDE::Mesh3DComponent skeletonMesh;
    skeletonMesh.vertexArray = nullptr;
    skeletonMesh.meshAsset = "engine:Skeleton/skeleton.obj";
    scene->AddComponent<FDE::Mesh3DComponent>(skeletonObj, skeletonMesh);
    return scene;
}

void ApplySceneViewCameraFromDescriptor(const FDE::ProjectDescriptor& desc, FDE::Camera3D& camera)
{
    if (!desc.sceneViewCamera3D.hasValue)
        return;
    const auto& c = desc.sceneViewCamera3D;
    camera.SetPositionYawPitch(glm::vec3(c.positionX, c.positionY, c.positionZ), c.yaw, c.pitch);
}

void SyncSceneViewCameraToDescriptor(FDE::Camera3D& camera, FDE::ProjectDescriptor& desc)
{
    const glm::vec3 p = camera.GetPosition();
    desc.sceneViewCamera3D.positionX = p.x;
    desc.sceneViewCamera3D.positionY = p.y;
    desc.sceneViewCamera3D.positionZ = p.z;
    desc.sceneViewCamera3D.yaw = camera.GetYaw();
    desc.sceneViewCamera3D.pitch = camera.GetPitch();
    desc.sceneViewCamera3D.hasValue = true;
}

void ResolveMesh3DInScene(FDE::Scene* scene, FDE::AssetManager* assets)
{
    if (!scene)
        return;
    FDE::AssetManager fallback;
    FDE::AssetManager* use = assets ? assets : &fallback;
    auto& reg = scene->GetRegistry();
    for (auto entity : reg.view<FDE::Mesh3DComponent>())
    {
        auto& mesh = reg.get<FDE::Mesh3DComponent>(entity);
        if (!mesh.vertexArray || mesh.vertexArray->GetIndexCount() == 0)
            use->ResolveMesh3D(mesh);
        use->ResolveMesh3DAlbedo(mesh);
    }
    for (auto entity : reg.view<FDE::SkyboxComponent>())
        use->ResolveSkybox(reg.get<FDE::SkyboxComponent>(entity));
}

} // namespace

namespace FDE
{

namespace
{
bool Transform3DNearEqual(const Transform3DComponent& a, const Transform3DComponent& b)
{
    constexpr float e = 1e-4f;
    auto near3 = [e](const glm::vec3& u, const glm::vec3& v) {
        return glm::abs(u.x - v.x) <= e && glm::abs(u.y - v.y) <= e && glm::abs(u.z - v.z) <= e;
    };
    return near3(a.position, b.position) && near3(a.rotation, b.rotation) && near3(a.scale, b.scale);
}

bool Transform2DNearEqual(const Transform2DComponent& a, const Transform2DComponent& b)
{
    constexpr float e = 1e-4f;
    return glm::abs(a.position.x - b.position.x) <= e && glm::abs(a.position.y - b.position.y) <= e
        && glm::abs(a.rotation - b.rotation) <= e && glm::abs(a.scale.x - b.scale.x) <= e
        && glm::abs(a.scale.y - b.scale.y) <= e;
}
} // namespace

void EditorApplication::PushUndo(EditorUndoEntry entry)
{
    m_redoStack.clear();
    m_undoStack.push_back(std::move(entry));
    if (m_undoStack.size() > 128u)
        m_undoStack.erase(m_undoStack.begin());
}

void EditorApplication::ApplyUndoEntryState(const EditorUndoEntry& entry, bool useBefore)
{
    std::visit(
        [this, useBefore](const auto& cmd) {
            using Cmd = std::decay_t<decltype(cmd)>;
            if (!cmd.object.IsValid())
                return;
            Scene* sc = cmd.object.GetScene();
            if (!sc)
                return;

            if constexpr (std::is_same_v<Cmd, EditorTransform3DUndo>)
            {
                Transform3DComponent* t = sc->GetComponent<Transform3DComponent>(cmd.object);
                if (t)
                    *t = useBefore ? cmd.before : cmd.after;
            }
            else if constexpr (std::is_same_v<Cmd, EditorTransform2DUndo>)
            {
                Transform2DComponent* t = sc->GetComponent<Transform2DComponent>(cmd.object);
                if (t)
                    *t = useBefore ? cmd.before : cmd.after;
            }
            else if constexpr (std::is_same_v<Cmd, EditorTagNameUndo>)
            {
                TagComponent* tag = sc->GetComponent<TagComponent>(cmd.object);
                if (tag)
                    tag->name = useBefore ? cmd.before : cmd.after;
            }
            else if constexpr (std::is_same_v<Cmd, EditorMeshAlbedoUndo>)
            {
                Mesh3DComponent* mesh = sc->GetComponent<Mesh3DComponent>(cmd.object);
                if (mesh)
                {
                    mesh->albedoTextureAsset = useBefore ? cmd.before : cmd.after;
                    mesh->albedoTexture.reset();
                    if (m_assetManager)
                        m_assetManager->ResolveMesh3DAlbedo(*mesh);
                }
            }
        },
        entry);
}

bool EditorApplication::UndoOne()
{
    if (m_undoStack.empty())
        return false;
    EditorUndoEntry e = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    ApplyUndoEntryState(e, true);
    m_redoStack.push_back(std::move(e));
    return true;
}

bool EditorApplication::RedoOne()
{
    if (m_redoStack.empty())
        return false;
    EditorUndoEntry e = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    ApplyUndoEntryState(e, false);
    m_undoStack.push_back(std::move(e));
    return true;
}

void EditorApplication::PushTransform3DUndo(const Object& obj, const Transform3DComponent& before,
                                            const Transform3DComponent& after)
{
    if (!obj.IsValid() || Transform3DNearEqual(before, after))
        return;
    PushUndo(EditorTransform3DUndo{obj, before, after});
}

void EditorApplication::PushTransform2DUndo(const Object& obj, const Transform2DComponent& before,
                                            const Transform2DComponent& after)
{
    if (!obj.IsValid() || Transform2DNearEqual(before, after))
        return;
    PushUndo(EditorTransform2DUndo{obj, before, after});
}

void EditorApplication::PushTagNameUndo(const Object& obj, std::string before, std::string after)
{
    if (!obj.IsValid() || before == after)
        return;
    PushUndo(EditorTagNameUndo{obj, std::move(before), std::move(after)});
}

void EditorApplication::PushMeshAlbedoUndo(const Object& obj, std::string before, std::string after)
{
    if (!obj.IsValid() || before == after)
        return;
    PushUndo(EditorMeshAlbedoUndo{obj, std::move(before), std::move(after)});
}

void EditorApplication::OnUndo()
{
    UndoOne();
}

void EditorApplication::OnRedo()
{
    RedoOne();
}

void EditorApplication::ProcessEditShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput)
        return;

    const bool mod = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    if (!mod)
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_Z, false) && !io.KeyShift)
        OnUndo();
    else if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
        OnRedo();
}

EditorApplication::EditorApplication(const std::string& initialProjectPath)
    : m_preferences(std::make_unique<EditorPreferences>())
    , m_showScene(m_preferences->GetShowScene())
    , m_showSceneTree(m_preferences->GetShowSceneTree())
    , m_showDetail(m_preferences->GetShowDetail())
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
            m_world = std::make_unique<World>();
            ProjectDescriptor desc;
            std::string err;
            if (ProjectDescriptor::LoadFromDirectory(projectRoot, desc, err, m_world.get()))
            {
                FileSystem::SetProjectRoot(projectRoot);
                m_projectDescriptor = desc;
                ApplySceneViewCameraFromDescriptor(desc, m_sceneCamera3D);
                m_preferences->SetLastProjectPath(projectRoot);
                m_contentViewCurrentPath.clear();
                SyncEditorActiveScenes();
                RefreshAssetPipeline();
                // Defer mesh GPU resolve to OnWindowCreated — OpenGL not ready in constructor
                FDE_LOG_CLIENT_INFO("Opened project from file: {} ({})", desc.name, projectRoot);
            }
            else
            {
                m_world.reset();
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
            return name == ".fproject" || name == "FordEditor.cfg" || name == "imgui.ini"
                || name == "imgui_runtime.ini";
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

void EditorApplication::SyncEditorActiveScenes()
{
    m_scene2D = nullptr;
    m_scene3D = nullptr;
    if (!m_world)
        return;
    Scene* active = m_world->GetActiveScene();
    m_scene2D = dynamic_cast<Scene2D*>(active);
    m_scene3D = dynamic_cast<Scene3D*>(active);
}

void EditorApplication::OnWindowCreated()
{
    Input::SetCurrentWindow(GetWindow());

    GetLayerStack().PushOverlay(std::make_unique<ImGuiLayer>());
    EditorConsole::Initialize();

    if (!m_world)
    {
        m_world = std::make_unique<World>();
        CreateDefaultScene3D(m_world.get());
        SyncEditorActiveScenes();
    }
    else
    {
        SyncEditorActiveScenes();
        if (!m_scene2D && !m_scene3D && m_world->GetSceneNames().empty())
        {
            CreateDefaultScene3D(m_world.get());
            SyncEditorActiveScenes();
        }
        ResolvePendingMeshes(m_world.get(), m_assetManager.get());
    }

    if (m_preferences->GetMaximized() && GetWindow())
        GetWindow()->Maximize();
}

void EditorApplication::OnBeforeImGuiNewFrame()
{
#if defined(_WIN32)
    ImGuiIO& io = ImGui::GetIO();
    if (m_sceneViewMouseCaptured)
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    else
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
#endif
}

void EditorApplication::ReleaseSceneViewMouseCapture()
{
#if defined(_WIN32)
    Win32ReleaseClipCursor();
#endif
    if (Window* w = GetWindow())
    {
        if (GLFWwindow* gw = w->GetGLFWWindow())
        {
            if (glfwRawMouseMotionSupported())
                glfwSetInputMode(gw, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
            glfwSetInputMode(gw, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    m_sceneViewMouseCaptured = false;
}

void EditorApplication::OnRunEnd()
{
    ReleaseSceneViewMouseCapture();
    if (Window* w = GetWindow())
        m_preferences->SetMaximized(w->IsMaximized());
    m_preferences->SetShowScene(m_showScene);
    m_preferences->SetShowSceneTree(m_showSceneTree);
    m_preferences->SetShowDetail(m_showDetail);
    m_preferences->SetShowContent(m_showContentView);
    m_preferences->SetShowConsole(m_showConsole);
    m_preferences->SetShowPreferences(m_showPreferences);
}

void EditorApplication::OnUpdate()
{
    if (m_assetManager)
        m_assetManager->ProcessAsyncUploads();
    if (m_world && m_sceneSimulationActive)
        m_world->OnUpdate(ImGui::GetIO().DeltaTime);
    if (!m_showScene && m_sceneViewMouseCaptured)
        ReleaseSceneViewMouseCapture();
    RenderMainUI();
    ProcessEditShortcuts();
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
    if (m_showSceneTree)
        RenderSceneTreeView(dockspace_id);
    if (m_showDetail)
        RenderDetailView(dockspace_id);
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
            if (ImGui::MenuItem("Play in Runtime", nullptr, false, FileSystem::HasProject()))
                OnPlayInRuntime();
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
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, CanUndo()))
                OnUndo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, CanRedo()))
                OnRedo();
            ImGui::Separator();
            if (ImGui::MenuItem("Preferences...")) { m_showPreferences = true; }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Assets"))
        {
            if (ImGui::MenuItem("Rescan Assets", nullptr, false, FileSystem::HasProject()))
                OnRescanAssets();
            if (ImGui::MenuItem("Build fdepack (Build/Game.fdepack)", nullptr, false, FileSystem::HasProject()))
                OnBuildAssetPack();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Scene", nullptr, &m_showScene);
            ImGui::MenuItem("Scene Tree", nullptr, &m_showSceneTree);
            ImGui::MenuItem("Detail", nullptr, &m_showDetail);
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
    desc.schemaVersion = 2;

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

    RefreshAssetPipeline();

    // Reset world to default for new project
    m_world = std::make_unique<World>();
    CreateDefaultScene3D(m_world.get());
    SyncEditorActiveScenes();
    m_selectedObject = Object{};
    m_scene3DGizmoState = Scene3DGizmoState{};

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

    m_world = std::make_unique<World>();
    ProjectDescriptor desc;
    std::string err;
    if (!ProjectDescriptor::LoadFromDirectory(selected, desc, err, m_world.get()))
    {
        m_world.reset();
        FDE_LOG_CLIENT_ERROR("Failed to load project: {}", err);
        return;
    }

    FileSystem::SetProjectRoot(selected);
    m_projectDescriptor = desc;
    ApplySceneViewCameraFromDescriptor(desc, m_sceneCamera3D);
    m_preferences->SetLastProjectPath(selected);
    m_contentViewCurrentPath.clear();
    RefreshAssetPipeline();
    SyncEditorActiveScenes();
    if (!m_scene2D && !m_scene3D && m_world->GetSceneNames().empty())
    {
        CreateDefaultScene3D(m_world.get());
        SyncEditorActiveScenes();
    }
    ResolvePendingMeshes(m_world.get(), m_assetManager.get());
    m_selectedObject = Object{};
    m_scene3DGizmoState = Scene3DGizmoState{};
    FDE_LOG_CLIENT_INFO("Opened project: {} ({})", desc.name, selected);
}

void EditorApplication::OnSaveProject()
{
    if (!FileSystem::HasProject() || !m_projectDescriptor)
        return;

    SyncSceneViewCameraToDescriptor(m_sceneCamera3D, *m_projectDescriptor);

    std::string root = FileSystem::GetProjectRoot();
    std::string err;
    if (!m_projectDescriptor->SaveToDirectory(root, err, m_world.get()))
    {
        FDE_LOG_CLIENT_ERROR("Failed to save project: {}", err);
        return;
    }
    FDE_LOG_CLIENT_INFO("Saved project: {}", root);
}

namespace
{

std::string ResolveFordRuntimeExecutablePath()
{
    namespace fs = std::filesystem;
    fs::path editorDir = FileSystem::GetExecutableDirectory();
#if defined(_WIN32)
    const char* runtimeName = "FordRuntime.exe";
#else
    const char* runtimeName = "FordRuntime";
#endif
    if (editorDir.empty())
        return {};
    fs::path sameDir = editorDir / runtimeName;
    if (fs::exists(sameDir))
        return sameDir.string();
    // Typical CMake/VS layout: Engine/Editor/<Config>/ next to Engine/RuntimeApp/<Config>/
    if (editorDir.has_filename())
    {
        fs::path engineDir = editorDir.parent_path().parent_path();
        fs::path sibling = engineDir / "RuntimeApp" / editorDir.filename() / runtimeName;
        if (fs::exists(sibling))
            return sibling.string();
    }
    return {};
}

#if defined(_WIN32)
std::wstring Utf8ToWide(const std::string& u8)
{
    if (u8.empty())
        return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), -1, nullptr, 0);
    if (n <= 0)
        return {};
    std::wstring out(static_cast<size_t>(n - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), -1, out.data(), n);
    return out;
}
#endif

} // namespace

void EditorApplication::OnPlayInRuntime()
{
    if (!FileSystem::HasProject() || !m_projectDescriptor)
    {
        FDE_LOG_CLIENT_WARN("Play in Runtime: no project open.");
        return;
    }

    OnSaveProject();

    namespace fs = std::filesystem;
    fs::path fproj = fs::path(FileSystem::GetProjectRoot()) / ProjectDescriptor::GetFileName();
    if (!fs::is_regular_file(fproj))
    {
        FDE_LOG_CLIENT_ERROR("Play in Runtime: missing project file {}", fproj.string());
        return;
    }

    std::string runtimeExe = ResolveFordRuntimeExecutablePath();
    if (runtimeExe.empty())
    {
        FDE_LOG_CLIENT_ERROR(
            "Play in Runtime: could not find FordRuntime next to the editor or under RuntimeApp/<Config>.");
        return;
    }

    const std::string fprojArg = fproj.string();

#if defined(_WIN32)
    std::wstring wExe = Utf8ToWide(runtimeExe);
    std::wstring wParams = L"\"" + Utf8ToWide(fprojArg) + L"\"";
    std::wstring wCwd = Utf8ToWide(FileSystem::GetProjectRoot());
    HINSTANCE shellResult =
        ShellExecuteW(nullptr, L"open", wExe.c_str(), wParams.c_str(), wCwd.c_str(), SW_SHOWNORMAL);
    if (reinterpret_cast<intptr_t>(shellResult) <= 32)
        FDE_LOG_CLIENT_ERROR("Play in Runtime: ShellExecute failed (code {})",
                             static_cast<int>(reinterpret_cast<intptr_t>(shellResult)));
#else
    pid_t pid = fork();
    if (pid < 0)
    {
        FDE_LOG_CLIENT_ERROR("Play in Runtime: fork failed.");
        return;
    }
    if (pid == 0)
    {
        std::string root = FileSystem::GetProjectRoot();
        if (chdir(root.c_str()) != 0)
            _exit(127);
        execl(runtimeExe.c_str(), "FordRuntime", fprojArg.c_str(), nullptr);
        _exit(127);
    }
#endif
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
    static float s_scene3dNavSensitivity = 1.0f;

    if (m_showPreferences && !s_wasOpen)
    {
        s_width = m_preferences->GetWindowWidth();
        s_height = m_preferences->GetWindowHeight();
        s_maximized = m_preferences->GetMaximized();
        s_contentIconSize = m_preferences->GetContentIconSize();
        s_scene3dNavSensitivity = m_preferences->GetScene3DNavSensitivity();
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
        if (ImGui::CollapsingHeader("3D Scene view", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextUnformatted("Navigation sensitivity");
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::SliderFloat("##Scene3DNavSens", &s_scene3dNavSensitivity, 0.05f, 5.0f, "%.2f",
                                   ImGuiSliderFlags_Logarithmic))
            {
                m_preferences->SetScene3DNavSensitivity(s_scene3dNavSensitivity);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Mouse look, WASD/QE move, and scroll dolly (1.0 = default)");
        }
    }
    ImGui::End();
}

void EditorApplication::RenderDetailView(ImGuiID dockspace_id)
{
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Detail"))
    {
        if (!m_selectedObject.IsValid())
        {
            ImGui::TextDisabled("No object selected");
            ImGui::TextDisabled("Select an object in the Scene Tree");
            ImGui::End();
            return;
        }

        Scene* scene = m_selectedObject.GetScene();
        if (!scene)
        {
            ImGui::TextDisabled("Invalid selection");
            ImGui::End();
            return;
        }

        // Tag component (name)
        if (auto* tag = scene->GetComponent<TagComponent>(m_selectedObject))
        {
            if (ImGui::CollapsingHeader("Tag", ImGuiTreeNodeFlags_DefaultOpen))
            {
                char buf[256];
                std::strncpy(buf, tag->name.c_str(), 255);
                buf[255] = '\0';
                if (ImGui::InputText("Name", buf, sizeof(buf)))
                    tag->name = buf;
                if (ImGui::IsItemActivated())
                    m_detailTagNameBaseline = tag->name;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (tag->name != m_detailTagNameBaseline)
                        PushTagNameUndo(m_selectedObject, m_detailTagNameBaseline, tag->name);
                }
            }
        }

        // Transform2D component
        if (auto* transform = scene->GetComponent<Transform2DComponent>(m_selectedObject))
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat2("Position", &transform->position.x, 0.05f);
                if (ImGui::IsItemActivated())
                    m_detailTf2WidgetBaseline = *transform;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (!Transform2DNearEqual(*transform, m_detailTf2WidgetBaseline))
                        PushTransform2DUndo(m_selectedObject, m_detailTf2WidgetBaseline, *transform);
                    m_detailTf2WidgetBaseline = *transform;
                }

                ImGui::DragFloat("Rotation", &transform->rotation, 0.01f, -3.14159f, 3.14159f, "%.3f rad");
                if (ImGui::IsItemActivated())
                    m_detailTf2WidgetBaseline = *transform;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (!Transform2DNearEqual(*transform, m_detailTf2WidgetBaseline))
                        PushTransform2DUndo(m_selectedObject, m_detailTf2WidgetBaseline, *transform);
                    m_detailTf2WidgetBaseline = *transform;
                }

                ImGui::DragFloat2("Scale", &transform->scale.x, 0.05f, 0.001f, 1000.0f);
                if (ImGui::IsItemActivated())
                    m_detailTf2WidgetBaseline = *transform;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (!Transform2DNearEqual(*transform, m_detailTf2WidgetBaseline))
                        PushTransform2DUndo(m_selectedObject, m_detailTf2WidgetBaseline, *transform);
                    m_detailTf2WidgetBaseline = *transform;
                }
            }
        }

        // Mesh2D component - show as read-only info for now
        if (scene->HasComponent<Mesh2DComponent>(m_selectedObject))
        {
            if (ImGui::CollapsingHeader("Mesh2D"))
            {
                ImGui::TextDisabled("(Mesh component - no editable properties)");
            }
        }

        if (auto* transform = scene->GetComponent<Transform3DComponent>(m_selectedObject))
        {
            if (ImGui::CollapsingHeader("Transform 3D", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("Position", &transform->position.x, 0.05f);
                if (ImGui::IsItemActivated())
                    m_detailTfWidgetBaseline = *transform;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (!Transform3DNearEqual(*transform, m_detailTfWidgetBaseline))
                        PushTransform3DUndo(m_selectedObject, m_detailTfWidgetBaseline, *transform);
                    m_detailTfWidgetBaseline = *transform;
                }

                ImGui::DragFloat3("Rotation (rad)", &transform->rotation.x, 0.01f);
                if (ImGui::IsItemActivated())
                    m_detailTfWidgetBaseline = *transform;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (!Transform3DNearEqual(*transform, m_detailTfWidgetBaseline))
                        PushTransform3DUndo(m_selectedObject, m_detailTfWidgetBaseline, *transform);
                    m_detailTfWidgetBaseline = *transform;
                }

                ImGui::DragFloat3("Scale", &transform->scale.x, 0.05f, 0.001f, 1000.0f);
                if (ImGui::IsItemActivated())
                    m_detailTfWidgetBaseline = *transform;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (!Transform3DNearEqual(*transform, m_detailTfWidgetBaseline))
                        PushTransform3DUndo(m_selectedObject, m_detailTfWidgetBaseline, *transform);
                    m_detailTfWidgetBaseline = *transform;
                }
            }
        }

        if (auto* mesh3 = scene->GetComponent<Mesh3DComponent>(m_selectedObject))
        {
            if (ImGui::CollapsingHeader("Mesh3D"))
            {
                ImGui::TextUnformatted("Mesh asset");
                ImGui::TextWrapped("%s", mesh3->meshAsset.c_str());
                char alb[512]{};
                std::strncpy(alb, mesh3->albedoTextureAsset.c_str(), sizeof(alb) - 1);
                alb[sizeof(alb) - 1] = '\0';
                if (ImGui::InputText("Albedo texture", alb, sizeof(alb)))
                {
                    mesh3->albedoTextureAsset = alb;
                    mesh3->albedoTexture.reset();
                }
                if (ImGui::IsItemActivated())
                    m_detailAlbedoBaseline = mesh3->albedoTextureAsset;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (mesh3->albedoTextureAsset != m_detailAlbedoBaseline)
                        PushMeshAlbedoUndo(m_selectedObject, m_detailAlbedoBaseline, mesh3->albedoTextureAsset);
                }
                if (m_assetManager)
                    m_assetManager->ResolveMesh3DAlbedo(*mesh3);
            }
        }

        if (auto* dirLight = scene->GetComponent<DirectionalLightComponent>(m_selectedObject))
        {
            if (ImGui::CollapsingHeader("Directional light"))
            {
                ImGui::TextDisabled("Direction: world-space travel of light rays (e.g. 0,-1,0 = downward).");
                ImGui::DragFloat3("Direction", &dirLight->direction.x, 0.02f);
                ImGui::DragFloat3("Color", &dirLight->color.x, 0.01f, 0.f, 4.f);
                ImGui::DragFloat("Intensity", &dirLight->intensity, 0.02f, 0.f, 16.f);
            }
        }

        if (auto* sky = scene->GetComponent<SkyboxComponent>(m_selectedObject))
        {
            if (ImGui::CollapsingHeader("Skybox"))
            {
                ImGui::TextDisabled("Horizontal cross cubemap PNG (Resources/). Example: engine:3d-space-skybox.png");
                char buf[512]{};
                std::strncpy(buf, sky->crossTextureAsset.c_str(), sizeof(buf) - 1);
                if (ImGui::InputText("Cross texture", buf, sizeof(buf)))
                {
                    sky->crossTextureAsset = buf;
                    sky->cubemap.reset();
                }
                if (m_assetManager)
                    m_assetManager->ResolveSkybox(*sky);
            }
        }
    }
    ImGui::End();
}

void EditorApplication::RenderSceneTreeView(ImGuiID dockspace_id)
{
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Tree"))
    {
        Scene* activeScene = m_world ? m_world->GetActiveScene() : nullptr;
        if (!activeScene)
        {
            ImGui::TextDisabled("No scene loaded");
            ImGui::End();
            return;
        }

        std::string sceneName = activeScene->GetName();
        ImGui::TextDisabled("%s", sceneName.c_str());
        ImGui::Separator();

        ImGui::BeginChild("SceneTreeList", ImVec2(0, 0), false);

        entt::registry& reg = activeScene->GetRegistry();
        for (auto entity : reg.view<entt::entity>())
        {
            const char* displayName = "Entity";
            if (auto* tag = reg.try_get<TagComponent>(entity))
                displayName = tag->name.c_str();

            Object obj(entity, activeScene);
            bool isSelected = m_selectedObject.IsValid() && m_selectedObject.GetEntity() == entity;

            if (ImGui::Selectable(displayName, isSelected))
            {
                m_selectedObject = obj;
            }
        }

        ImGui::EndChild();
    }
    ImGui::End();
}

void EditorApplication::ProcessSceneViewportCameraInput(bool allowSceneCameraInput, uint32_t width,
                                                         uint32_t height)
{
    if (!allowSceneCameraInput || width == 0 || height == 0)
        return;

    ImGuiIO& io = ImGui::GetIO();
    Scene* activeNav = m_world ? m_world->GetActiveScene() : nullptr;
    const bool scene3DNav = activeNav
                            && (Scene3D::HasMesh3DDrawables(*activeNav)
                                || Scene3D::HasSkyboxConfigured(*activeNav));
    if (!scene3DNav)
        m_sceneCamera3D.ClearMouseLookSmoothing();

    if (scene3DNav)
    {
        const float navSens = m_preferences->GetScene3DNavSensitivity();
        const bool rmbNav = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        if (rmbNav)
        {
            m_sceneCamera3D.ApplyMouseLook(io.MouseDelta.x, io.MouseDelta.y, navSens);
            FDE::Window* win = GetWindow();
            float fwd = 0.0f;
            float right = 0.0f;
            float up = 0.0f;
            if (GlfwNavKeyDown(win, GLFW_KEY_W) || GlfwNavKeyDown(win, GLFW_KEY_UP))
                fwd += 1.0f;
            if (GlfwNavKeyDown(win, GLFW_KEY_S) || GlfwNavKeyDown(win, GLFW_KEY_DOWN))
                fwd -= 1.0f;
            if (GlfwNavKeyDown(win, GLFW_KEY_D) || GlfwNavKeyDown(win, GLFW_KEY_RIGHT))
                right -= 1.0f;
            if (GlfwNavKeyDown(win, GLFW_KEY_A) || GlfwNavKeyDown(win, GLFW_KEY_LEFT))
                right += 1.0f;
            if (GlfwNavKeyDown(win, GLFW_KEY_Q))
                up += 1.0f;
            if (GlfwNavKeyDown(win, GLFW_KEY_E))
                up -= 1.0f;
            m_sceneCamera3D.ApplyFlyMovement(fwd, right, up, io.DeltaTime, navSens);
        }
        else
            m_sceneCamera3D.ClearMouseLookSmoothing();

        if (io.MouseWheel != 0.0f)
            m_sceneCamera3D.Dolly(io.MouseWheel, navSens);
    }
    else if (m_scene2D)
    {
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
            float focusX = mousePos.x - m_sceneViewLastImageMin.x;
            float focusY = mousePos.y - m_sceneViewLastImageMin.y;
            m_sceneCamera.ZoomAt(wheel * 0.2f, focusX, focusY, width, height);
        }
    }
}

void EditorApplication::RenderSceneGridOverlay(ImVec2 itemMin, ImVec2 itemMax, uint32_t viewportWidth,
                                              uint32_t viewportHeight)
{
    if (!m_preferences->GetShowSceneGrid() || viewportWidth == 0 || viewportHeight == 0)
        return;

    float gridSize = m_preferences->GetSceneGridSize();
    if (gridSize <= 0.001f)
        gridSize = 0.5f;

    float aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    float halfWidth = 2.0f / m_sceneCamera.GetZoom();
    float halfHeight = halfWidth / aspect;
    glm::vec2 pos = m_sceneCamera.GetPosition();
    float left = pos.x - halfWidth;
    float right = pos.x + halfWidth;
    float bottom = pos.y - halfHeight;
    float top = pos.y + halfHeight;

    float startX = std::floor(left / gridSize) * gridSize;
    float endX = std::ceil(right / gridSize) * gridSize;
    float startY = std::floor(bottom / gridSize) * gridSize;
    float endY = std::ceil(top / gridSize) * gridSize;

    float rectW = itemMax.x - itemMin.x;
    float rectH = itemMax.y - itemMin.y;
    auto worldToScreen = [&](float wx, float wy) -> ImVec2 {
        float sx = itemMin.x + (wx - left) / (right - left) * rectW;
        float sy = itemMin.y + (top - wy) / (top - bottom) * rectH;
        return ImVec2(sx, sy);
    };

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 gridColor = IM_COL32(128, 128, 140, 200);

    for (float y = startY; y <= endY; y += gridSize)
    {
        ImVec2 p0 = worldToScreen(left, y);
        ImVec2 p1 = worldToScreen(right, y);
        drawList->AddLine(p0, p1, gridColor, 1.0f);
    }
    for (float x = startX; x <= endX; x += gridSize)
    {
        ImVec2 p0 = worldToScreen(x, bottom);
        ImVec2 p1 = worldToScreen(x, top);
        drawList->AddLine(p0, p1, gridColor, 1.0f);
    }
}

namespace
{

/// Right-pointing play triangle; returns true if clicked while \p enabled.
bool SceneViewToolbarPlayButton(bool enabled)
{
    const float side = ImGui::GetFrameHeight();
    if (!enabled)
        ImGui::BeginDisabled();
    ImGui::InvisibleButton("##ScenePlayRuntime", ImVec2(side, side));
    if (!enabled)
        ImGui::EndDisabled();

    const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();

    if (hovered && enabled)
        dl->AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_ButtonHovered), 3.0f);
    else if (hovered)
        dl->AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_FrameBg), 2.0f);

    const ImVec2 c(0.5f * (min.x + max.x), 0.5f * (min.y + max.y));
    const float r = side * 0.32f;
    const ImU32 triCol = enabled ? IM_COL32(236, 236, 244, 255) : IM_COL32(110, 110, 120, 255);
    const ImVec2 v0(c.x - r * 0.5f, c.y - r);
    const ImVec2 v1(c.x - r * 0.5f, c.y + r);
    const ImVec2 v2(c.x + r * 0.85f, c.y);
    dl->AddTriangleFilled(v0, v1, v2, triCol);

    if (hovered)
    {
        if (enabled)
            ImGui::SetTooltip("Play in Runtime");
        else
            ImGui::SetTooltip("Open a project to play");
    }

    return enabled && ImGui::IsItemClicked(ImGuiMouseButton_Left);
}

} // namespace

void EditorApplication::RenderSceneView(ImGuiID dockspace_id)
{
    ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene"))
    {
        bool showGrid = m_preferences->GetShowSceneGrid();
        if (ImGui::Checkbox("Grid", &showGrid))
            m_preferences->SetShowSceneGrid(showGrid);
        ImGui::SameLine();
        float gridSize = m_preferences->GetSceneGridSize();
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::DragFloat("##GridSize", &gridSize, 0.05f, 0.1f, 10.0f, "%.2f"))
        {
            gridSize = std::max(0.1f, std::min(10.0f, gridSize));
            m_preferences->SetSceneGridSize(gridSize);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Grid cell size (2D overlay / 3D XZ plane)");

        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Transform");
        ImGui::SameLine();
        {
            int tfm = m_preferences->GetScene3DTransformMode();
            ImGui::RadioButton("Pos##SceneTfm", &tfm, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Rot##SceneTfm", &tfm, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Scl##SceneTfm", &tfm, 2);
            if (tfm != m_preferences->GetScene3DTransformMode())
                m_preferences->SetScene3DTransformMode(tfm);
        }

        ImGui::Spacing();
        if (SceneViewToolbarPlayButton(FileSystem::HasProject()))
            OnPlayInRuntime();
        ImGui::SameLine();
        ImGui::Checkbox("Simulate", &m_sceneSimulationActive);

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
                Scene* activeForDraw = m_world ? m_world->GetActiveScene() : nullptr;
                const bool hoverForInput =
                    m_sceneViewLayoutCached
                    && ImGui::IsMouseHoveringRect(m_sceneViewLastImageMin, m_sceneViewLastImageMax, false);
                ProcessSceneViewportCameraInput(m_sceneViewMouseCaptured || hoverForInput, width, height);

                if (m_sceneViewLayoutCached && activeForDraw && Scene3D::HasMesh3DDrawables(*activeForDraw))
                {
                    const Scene3DTransformMode tfMode =
                        static_cast<Scene3DTransformMode>(m_preferences->GetScene3DTransformMode());
                    const bool gizmoInteract =
                        hoverForInput || m_sceneViewMouseCaptured || m_scene3DGizmoState.dragging;
                    Scene3D_UpdateGizmoInteraction(tfMode, *activeForDraw, m_selectedObject, m_sceneCamera3D,
                                                   width, height, m_sceneViewLastImageMin, m_sceneViewLastImageMax,
                                                   m_scene3DGizmoState, gizmoInteract);
                    if (m_scene3DGizmoState.gizmoDragReleasedUndoPending)
                    {
                        if (m_selectedObject.IsValid() && activeForDraw->IsValid(m_selectedObject)
                            && activeForDraw->HasComponent<Transform3DComponent>(m_selectedObject))
                        {
                            if (auto* trUndo =
                                    activeForDraw->GetComponent<Transform3DComponent>(m_selectedObject))
                                PushTransform3DUndo(m_selectedObject,
                                                      m_scene3DGizmoState.transformBeforeGizmoDrag, *trUndo);
                        }
                        m_scene3DGizmoState.gizmoDragReleasedUndoPending = false;
                    }
                }

                m_sceneViewport->Bind();

                Renderer::SetClearColor(0.15f, 0.15f, 0.18f, 1.0f);
                Renderer::Clear();

                Renderer::UseDefaultShader();
                glEnable(GL_DEPTH_TEST);

                Scene3D* scene3D = activeForDraw ? dynamic_cast<Scene3D*>(activeForDraw) : nullptr;
                if (scene3D)
                {
                    ResolveMesh3DInScene(activeForDraw, m_assetManager.get());
                    Scene3D::RenderSkyboxIfAny(*scene3D, m_sceneCamera3D, width, height, m_assetManager.get());
                }

                if (activeForDraw && Scene3D::HasMesh3DDrawables(*activeForDraw))
                {
                    if (showGrid)
                        DrawEditorSceneGrid3D(m_sceneCamera3D, width, height, gridSize);
                    Scene3D::RenderMesh3DEntities(*activeForDraw, m_sceneCamera3D, width, height,
                                                  m_assetManager.get());

                    const glm::mat4 viewMat = m_sceneCamera3D.GetViewMatrix();
                    const glm::mat4 projMat = m_sceneCamera3D.GetProjectionMatrix(width, height);
                    if (m_selectedObject.IsValid() && activeForDraw == m_selectedObject.GetScene())
                    {
                        if (auto* mesh3 = activeForDraw->GetComponent<Mesh3DComponent>(m_selectedObject))
                        {
                            if (auto* tr3 = activeForDraw->GetComponent<Transform3DComponent>(m_selectedObject))
                            {
                                if (mesh3->vertexArray && mesh3->vertexArray->GetIndexCount() > 0)
                                {
                                    glm::mat4 model = glm::translate(glm::mat4(1.0f), tr3->position);
                                    model = glm::rotate(model, tr3->rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
                                    model = glm::rotate(model, tr3->rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
                                    model = glm::rotate(model, tr3->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
                                    model = glm::scale(model, tr3->scale);
                                    DrawMesh3DSelectionOutline(model, viewMat, projMat, mesh3->vertexArray);
                                }
                            }
                        }
                        const Scene3DTransformMode tfMode =
                            static_cast<Scene3DTransformMode>(m_preferences->GetScene3DTransformMode());
                        DrawScene3DGizmo(tfMode, *activeForDraw, m_selectedObject, m_sceneCamera3D, width, height,
                                         viewMat, projMat, m_scene3DGizmoState);
                    }
                }
                else if (!scene3D && m_scene2D)
                {
                    m_scene2D->Render(m_sceneCamera, width, height);
                }

                glDisable(GL_DEPTH_TEST);

                m_sceneViewport->Unbind();

                ImGui::Image(m_sceneViewport->GetColorAttachmentTextureId(), viewportSize,
                             ImVec2(0, 1), ImVec2(1, 0));

                ImVec2 imgMin = ImGui::GetItemRectMin();
                ImVec2 imgMax = ImGui::GetItemRectMax();
                if (m_scene2D)
                    RenderSceneGridOverlay(imgMin, imgMax, width, height);

                m_sceneViewLastImageMin = imgMin;
                m_sceneViewLastImageMax = imgMax;
                m_sceneViewLastWidth = width;
                m_sceneViewLastHeight = height;
                m_sceneViewLayoutCached = true;

                ImGui::SetCursorScreenPos(imgMin);
                ImGui::InvisibleButton("##SceneViewport", viewportSize);
                const bool viewportItemHovered =
                    ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

                if (activeForDraw && Scene3D::HasMesh3DDrawables(*activeForDraw)
                    && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && viewportItemHovered
                    && !m_sceneViewMouseCaptured)
                {
                    const Scene3DTransformMode tfMode =
                        static_cast<Scene3DTransformMode>(m_preferences->GetScene3DTransformMode());
                    ImGuiIO& ioPick = ImGui::GetIO();
                    Scene3D_OnViewportPrimaryClick(tfMode, *activeForDraw, m_selectedObject, m_sceneCamera3D, width,
                                                   height, imgMin, imgMax,
                                                   glm::vec2(ioPick.MousePos.x, ioPick.MousePos.y),
                                                   m_scene3DGizmoState);
                }

                Window* appWin = GetWindow();
                GLFWwindow* gw = appWin ? appWin->GetGLFWWindow() : nullptr;
                if (!gw && m_sceneViewMouseCaptured)
                    ReleaseSceneViewMouseCapture();
                else if (gw && !glfwGetWindowAttrib(gw, GLFW_FOCUSED))
                {
                    if (m_sceneViewMouseCaptured)
                        ReleaseSceneViewMouseCapture();
                }
                else if (gw)
                {
                    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
                    {
                        if (m_sceneViewMouseCaptured)
                            ReleaseSceneViewMouseCapture();
                    }
                    else
                    {
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && viewportItemHovered)
                            m_sceneViewMouseCaptured = true;
                        if (m_sceneViewMouseCaptured)
                        {
#if defined(_WIN32)
                            glfwSetInputMode(gw, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                            Win32ClipCursorToSceneRect(gw, imgMin.x, imgMin.y, imgMax.x, imgMax.y);
#else
                            glfwSetInputMode(gw, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#endif
                            if (glfwRawMouseMotionSupported())
                                glfwSetInputMode(gw, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                        }
                    }
                }
            }
            else
            {
                m_sceneViewLayoutCached = false;
                if (m_sceneViewMouseCaptured)
                    ReleaseSceneViewMouseCapture();
            }
        }
        else
        {
            m_sceneViewLayoutCached = false;
            if (m_sceneViewMouseCaptured)
                ReleaseSceneViewMouseCapture();
        }
    }
    ImGui::End();
}

void EditorApplication::RefreshAssetPipeline()
{
    if (!FileSystem::HasProject())
    {
        if (m_assetManager)
            m_assetManager->Shutdown();
        m_assetManager.reset();
        return;
    }
    if (!m_assetManager)
        m_assetManager = std::make_unique<AssetManager>();
    std::string err;
    if (!AssetDatabase::EnsureLibraryLayout(FileSystem::GetProjectRoot(), err))
        FDE_LOG_CLIENT_WARN("Asset layout: {}", err);
    err.clear();
    if (!AssetDatabase::RescanAssets(FileSystem::GetProjectRoot(), err))
        FDE_LOG_CLIENT_WARN("Asset rescan: {}", err);
    m_assetManager->Initialize(FileSystem::GetProjectRoot());
}

void EditorApplication::OnRescanAssets()
{
    if (!FileSystem::HasProject())
        return;
    std::string err;
    AssetDatabase::EnsureLibraryLayout(FileSystem::GetProjectRoot(), err);
    AssetDatabase::RescanAssets(FileSystem::GetProjectRoot(), err);
    if (m_assetManager)
        m_assetManager->Initialize(FileSystem::GetProjectRoot());
    ResolvePendingMeshes(m_world.get(), m_assetManager.get());
    FDE_LOG_CLIENT_INFO("Asset rescan finished.");
}

void EditorApplication::OnBuildAssetPack()
{
    if (!FileSystem::HasProject())
        return;
    std::string err;
    if (AssetDatabase::BuildFdepack(FileSystem::GetProjectRoot(), "Build/Game.fdepack", err))
        FDE_LOG_CLIENT_INFO("Built Build/Game.fdepack");
    else
        FDE_LOG_CLIENT_ERROR("fdepack failed: {}", err);
}

bool EditorApplication::Initialize()
{
    return true;
}

} // namespace FDE
