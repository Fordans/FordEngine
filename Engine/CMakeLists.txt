add_library(FordEngine SHARED)
target_compile_definitions(FordEngine PRIVATE FordEngine_EXPORTS)

target_include_directories(FordEngine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_sources(FordEngine PRIVATE 
    src/Core/Application.cpp
    src/Core/Log.cpp
    src/Window/Window.cpp
)

# Precompiled headers (optional, to speed up compilation)
# target_precompile_headers(FordEngine PRIVATE include/FDE/pch.hpp)

target_link_libraries(FordEngine
    PUBLIC
        spdlog::spdlog
        glfw
)
