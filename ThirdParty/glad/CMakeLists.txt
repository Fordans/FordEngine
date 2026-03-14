# GLAD - OpenGL loader
add_library(glad STATIC
    src/glad.c
)

target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# No external dependencies - uses platform APIs (Windows, X11, etc.) for loading
