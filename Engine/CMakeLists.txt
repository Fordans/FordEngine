add_library(FordEngine SHARED)
target_compile_definitions(FordEngine PRIVATE FordEngine_EXPORTS)

target_sources(FordEngine PRIVATE 
    src/Asset/AssetId.cpp
    src/Asset/AssetType.cpp
    src/Asset/AssetRegistry.cpp
    src/Asset/AssetDatabase.cpp
    src/Asset/AssetManager.cpp
    src/Asset/AssetPack.cpp
    src/Asset/AssetLoadQueue.cpp
    src/Asset/MeshImporter.cpp
    src/Asset/Texture2DResource.cpp
    src/Core/Application.cpp
    src/Scene/Object.cpp
    src/Scene/Scene.cpp
    src/Scene/Scene2D.cpp
    src/Scene/Scene3D.cpp
    src/Scene/World.cpp
    src/Project/ProjectDescriptor.cpp
    src/Core/FileSystem.cpp
    src/Core/Layer.cpp
    src/Core/LayerStack.cpp
    src/Core/Log.cpp
    src/Core/StbImage.cpp
    src/ImGui/ImGuiContext.cpp
    src/ImGui/ImGuiLayer.cpp
    src/Window/Window.cpp
    src/Editor/EditorApplication.cpp
    src/Editor/EditorConsole.cpp
    src/Editor/EditorPreferences.cpp
    src/Editor/EditorScene3DTools.cpp
    src/Renderer/Camera2D.cpp
    src/Renderer/Camera3D.cpp
    src/Renderer/RenderCommand.cpp
    src/Renderer/Renderer.cpp
    src/Renderer/Shader.cpp
    src/Renderer/VertexArray.cpp
    src/Renderer/VertexBuffer.cpp
    src/Renderer/Viewport.cpp
    src/Renderer/OpenGL/OpenGLRendererAPI.cpp
    src/Renderer/OpenGL/OpenGLShader.cpp
    src/Renderer/OpenGL/OpenGLVertexArray.cpp
    src/Renderer/OpenGL/OpenGLVertexBuffer.cpp
    src/Renderer/OpenGL/OpenGLViewport.cpp
)

target_precompile_headers(FordEngine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/FDE/pch.hpp)

target_include_directories(FordEngine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${CMAKE_SOURCE_DIR}/ThirdParty/entt
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/ThirdParty/fds
        ${CMAKE_SOURCE_DIR}/ThirdParty/stb
)

target_link_libraries(FordEngine
    PUBLIC
        spdlog::spdlog
        glfw
        glad
        imgui
        glm
        json11
    PRIVATE
        assimp::assimp
)

if(WIN32)
    target_link_libraries(FordEngine PRIVATE ole32)
endif()

add_subdirectory(Editor)
