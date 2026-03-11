add_executable(FDEditor)

target_include_directories(FDEditor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_sources(FDEditor PRIVATE
    src/main.cpp
)

target_link_libraries(FDEditor
    PRIVATE
        FDEngine
)
