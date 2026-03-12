add_executable(FordEditor)

target_include_directories(FordEditor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_sources(FordEditor PRIVATE
    src/main.cpp
    src/EditorApplication.cpp
)

target_link_libraries(FordEditor
    PRIVATE
        FordEngine
)
