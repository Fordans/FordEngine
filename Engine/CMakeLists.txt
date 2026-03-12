add_library(FordEngine SHARED)
target_compile_definitions(FordEngine PRIVATE FordEngine_EXPORTS)

target_include_directories(FordEngine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set(ENGINE_SOURCES
    src/Core/Application.cpp
    src/Core/Log.cpp
    # src/Math/Vector.cpp
    # src/Platform/Window.cpp
    # src/Graphics/Renderer.cpp
    # src/Input/Input.cpp
    # src/Scene/Scene.cpp
)

target_sources(FordEngine PRIVATE ${ENGINE_SOURCES})

# Precompiled headers (optional, to speed up compilation)
# target_precompile_headers(FordEngine PRIVATE include/FDE/pch.hpp)

target_link_libraries(FordEngine
    PUBLIC
        spdlog::spdlog
)
