# Dear ImGui - GLFW + OpenGL3 backend
find_package(OpenGL REQUIRED)

add_library(imgui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

# Use GLAD as OpenGL loader (instead of imgui_impl_opengl3_loader.h)
set(GLAD_INCLUDE_PRE "${CMAKE_SOURCE_DIR}/ThirdParty/glad/include/glad/glad.h")
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
target_link_libraries(imgui PUBLIC glad)
if(MSVC)
    target_compile_options(imgui PRIVATE /FI"${GLAD_INCLUDE_PRE}")
else()
    target_compile_options(imgui PRIVATE -include "${GLAD_INCLUDE_PRE}")
endif()

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)
