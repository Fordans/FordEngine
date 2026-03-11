add_library(FDEngine SHARED)
target_compile_definitions(FDEngine PRIVATE FDEngine_EXPORTS)

target_include_directories(FDEngine
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

target_sources(FDEngine PRIVATE ${ENGINE_SOURCES})

# Precompiled headers (optional, to speed up compilation)
# target_precompile_headers(FDEngine PRIVATE include/FDE/pch.hpp)

target_link_libraries(FDEngine
    PUBLIC
        spdlog::spdlog
)
