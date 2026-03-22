# ThirdParty
# =====================================

# -----------------------------------------------------------------------------
# spdlog
# -----------------------------------------------------------------------------
set(SPDLOG_MASTER_PROJECT OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(spdlog)

# -----------------------------------------------------------------------------
# GLAD - OpenGL loader
# -----------------------------------------------------------------------------
add_subdirectory(glad)

# -----------------------------------------------------------------------------
# GLFW
# -----------------------------------------------------------------------------
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# -----------------------------------------------------------------------------
# ImGui
# -----------------------------------------------------------------------------
add_subdirectory(imgui)

# -----------------------------------------------------------------------------
# GLM - OpenGL Mathematics (header-only)
# -----------------------------------------------------------------------------
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glm/include)

# -----------------------------------------------------------------------------
# json11 - Lightweight JSON library for C++11
# -----------------------------------------------------------------------------
set(JSON11_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(json11)

# -----------------------------------------------------------------------------
# Assimp — static library, no tools/tests/samples/install
# -----------------------------------------------------------------------------
set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_IGNORE_GIT_HASH ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(ASSIMP_OPT_BUILD_PACKAGES OFF CACHE BOOL "" FORCE)
add_subdirectory(assimp)
