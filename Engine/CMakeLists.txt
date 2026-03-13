add_library(FordEngine SHARED)
target_compile_definitions(FordEngine PRIVATE FordEngine_EXPORTS)

target_sources(FordEngine PRIVATE 
    src/Core/Application.cpp
    src/Core/Log.cpp
    src/ImGui/ImGuiContext.cpp
    src/Window/Window.cpp
    src/Editor/EditorApplication.cpp
)

# Precompiled headers (optional, to speed up compilation)
# target_precompile_headers(FordEngine PRIVATE include/FDE/pch.hpp)

target_include_directories(FordEngine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(FordEngine
    PUBLIC
        spdlog::spdlog
        glfw
        imgui
)

add_subdirectory(Editor)
