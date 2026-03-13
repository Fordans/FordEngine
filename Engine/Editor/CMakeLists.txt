# FordEditor - Editor executable
add_executable(FordEditor ${CMAKE_CURRENT_SOURCE_DIR}/../src/Editor/main.cpp)
target_link_libraries(FordEditor PRIVATE FordEngine)
