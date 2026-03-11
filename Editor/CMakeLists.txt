# FDEditor - 基于 Engine 的可视化编辑器
# 链接 FDEngine 动态库，提供场景编辑、资源管理等 GUI 功能

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
