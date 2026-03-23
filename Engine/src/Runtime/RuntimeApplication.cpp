#include "FDE/pch.hpp"
#include "FDE/Asset/AssetManager.hpp"
#include "FDE/Runtime/RuntimeApplication.hpp"
#include "FDE/Runtime/RuntimeDebugLayer.hpp"
#include "FDE/Runtime/RuntimeMeshResolve.hpp"
#include "FDE/Core/Input.hpp"
#include "FDE/Core/Log.hpp"
#include "FDE/ImGui/ImGuiLayer.hpp"
#include "FDE/Scene/Scene.hpp"
#include "FDE/Scene/Scene3D.hpp"
#include "FDE/Scene/World.hpp"
#include "FDE/Window/Window.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"

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

} // namespace

// Default for debug overlay; overridden by CMake option FDE_RUNTIME_DEBUG.
#ifndef FDE_RUNTIME_DEBUG_DEFAULT
#define FDE_RUNTIME_DEBUG_DEFAULT 1
#endif

namespace FDE
{

RuntimeApplication::RuntimeApplication(std::string fprojectPath, std::string fdepackPath)
    : m_fprojectPath(std::move(fprojectPath))
    , m_fdepackPath(std::move(fdepackPath))
#if FDE_RUNTIME_DEBUG_DEFAULT
    , m_debugUIEnabled(true)
#else
    , m_debugUIEnabled(false)
#endif
{
    std::string err;
    if (m_fprojectPath.empty())
    {
        FDE_LOG_CLIENT_ERROR("RuntimeApplication: empty .fproject path");
        return;
    }
    if (m_session.LoadFromFProjectFile(m_fprojectPath, err, m_fdepackPath))
    {
        m_loadOk = true;
        FDE_LOG_CLIENT_INFO("Runtime: loaded project '{}'", m_session.GetDescriptor().name);
    }
    else
    {
        FDE_LOG_CLIENT_ERROR("RuntimeApplication: {}", err);
    }
}

RuntimeApplication::~RuntimeApplication() = default;

WindowSpec RuntimeApplication::GetWindowSpec() const
{
    WindowSpec spec;
    spec.title = m_loadOk ? ("Ford Runtime — " + m_session.GetDescriptor().name) : "Ford Runtime";
    spec.decorated = true;
    return spec;
}

void RuntimeApplication::OnWindowCreated()
{
    Input::SetCurrentWindow(GetWindow());

    static std::string imguiIni = "imgui_runtime.ini";
    ImGui::GetIO().IniFilename = imguiIni.c_str();

    GetLayerStack().PushOverlay(
        std::make_unique<RuntimeDebugLayer>(&m_session, &m_smoothedFps, &m_debugUIEnabled));
    GetLayerStack().PushOverlay(std::make_unique<ImGuiLayer>());

    if (!m_loadOk && GetWindow())
        GetWindow()->RequestClose();
    else if (m_loadOk)
        ResolvePendingMeshes(m_session.GetWorld(), m_session.GetAssetManager());
}

void RuntimeApplication::OnBeforeImGuiNewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    if (m_debugUIEnabled)
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    else
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
}

void RuntimeApplication::OnUpdate()
{
    Window* win = GetWindow();
    if (!win || !m_loadOk)
        return;

    GLFWwindow* w = win->GetGLFWWindow();
    if (!w)
        return;

    double now = glfwGetTime();
    float dt = m_lastFrameTime > 0.0 ? static_cast<float>(now - m_lastFrameTime) : (1.0f / 60.0f);
    m_lastFrameTime = now;

    if (Input::IsKeyDown(GLFW_KEY_F3) && !m_f3WasDown)
        m_debugUIEnabled = !m_debugUIEnabled;
    m_f3WasDown = Input::IsKeyDown(GLFW_KEY_F3);

    if (AssetManager* am = m_session.GetAssetManager())
        am->ProcessAsyncUploads();

    World* world = m_session.GetWorld();
    if (!world)
        return;

    ResolvePendingMeshes(world, m_session.GetAssetManager());
    world->OnUpdate(dt);

    ImGuiIO& io = ImGui::GetIO();
    const bool blockGameMouse = m_debugUIEnabled && io.WantCaptureMouse;
    const bool blockGameKb = m_debugUIEnabled && io.WantCaptureKeyboard;

    Camera3D& cam = m_session.GetCamera();

    Scene* activeNav = world->GetActiveScene();
    const bool scene3DNav = activeNav && Scene3D::HasMesh3DDrawables(*activeNav);
    const float navSens = m_session.GetScene3DNavSensitivity();

    if (scene3DNav)
    {
        if (!blockGameMouse)
        {
            if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
            {
                double cx = 0.0;
                double cy = 0.0;
                glfwGetCursorPos(w, &cx, &cy);
                if (!m_rmbDown)
                {
                    m_rmbDown = true;
                    m_lastCursorX = cx;
                    m_lastCursorY = cy;
                }
                else
                {
                    float dx = static_cast<float>(cx - m_lastCursorX);
                    float dy = static_cast<float>(cy - m_lastCursorY);
                    cam.ApplyMouseLook(dx, dy, navSens);
                    m_lastCursorX = cx;
                    m_lastCursorY = cy;
                }
            }
            else
            {
                m_rmbDown = false;
                cam.ClearMouseLookSmoothing();
            }

            if (io.MouseWheel != 0.0f)
                cam.Dolly(io.MouseWheel, navSens);
        }
        else
        {
            m_rmbDown = false;
            cam.ClearMouseLookSmoothing();
        }
    }
    else
    {
        m_rmbDown = false;
        cam.ClearMouseLookSmoothing();
    }

    if (scene3DNav && !blockGameKb)
    {
        float fwd = 0.0f;
        float right = 0.0f;
        float up = 0.0f;
        if (Input::IsKeyDown(GLFW_KEY_W) || Input::IsKeyDown(GLFW_KEY_UP))
            fwd += 1.0f;
        if (Input::IsKeyDown(GLFW_KEY_S) || Input::IsKeyDown(GLFW_KEY_DOWN))
            fwd -= 1.0f;
        if (Input::IsKeyDown(GLFW_KEY_D) || Input::IsKeyDown(GLFW_KEY_RIGHT))
            right -= 1.0f;
        if (Input::IsKeyDown(GLFW_KEY_A) || Input::IsKeyDown(GLFW_KEY_LEFT))
            right += 1.0f;
        if (Input::IsKeyDown(GLFW_KEY_Q))
            up += 1.0f;
        if (Input::IsKeyDown(GLFW_KEY_E))
            up -= 1.0f;
        cam.ApplyFlyMovement(fwd, right, up, dt, navSens);
    }

    const float instFps = (dt > 1.0e-6f) ? (1.0f / dt) : 0.0f;
    m_smoothedFps = m_smoothedFps * 0.95f + instFps * 0.05f;
}

void RuntimeApplication::OnRender()
{
    if (!m_loadOk)
        return;

    Window* win = GetWindow();
    World* world = m_session.GetWorld();
    if (!win || !world)
        return;

    int fbw = 0;
    int fbh = 0;
    win->GetFramebufferSize(fbw, fbh);
    if (fbw <= 0 || fbh <= 0)
        return;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_DEPTH_BUFFER_BIT);

    Scene* active = world->GetActiveScene();
    auto* scene3d = dynamic_cast<Scene3D*>(active);
    if (scene3d)
        scene3d->Render(m_session.GetCamera(), static_cast<uint32_t>(fbw), static_cast<uint32_t>(fbh),
                        m_session.GetAssetManager());
}

void RuntimeApplication::OnRunEnd()
{
    m_session.Shutdown();
}

} // namespace FDE
