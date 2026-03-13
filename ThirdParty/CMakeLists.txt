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
