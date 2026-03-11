# Ford Engine - 核心引擎动态库
# 提供渲染、场景、输入等核心能力，供 Editor 和可能的 Runtime 使用

add_library(FDEngine SHARED)
target_compile_definitions(FDEngine PRIVATE FDEngine_EXPORTS)

target_include_directories(FDEngine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 源文件按模块组织，便于逐步添加
set(ENGINE_SOURCES
    src/Core/Application.cpp
    src/Core/Log.cpp
    # src/Math/Vector.cpp      # 后续添加
    # src/Platform/Window.cpp  # 后续添加
    # src/Graphics/Renderer.cpp
    # src/Input/Input.cpp
    # src/Scene/Scene.cpp
)

target_sources(FDEngine PRIVATE ${ENGINE_SOURCES})

# 预编译头（可选，提升编译速度）
# target_precompile_headers(FDEngine PRIVATE include/FDE/pch.hpp)

target_link_libraries(FDEngine
    PUBLIC
        # 平台相关库在 Platform 模块中按需添加
)
