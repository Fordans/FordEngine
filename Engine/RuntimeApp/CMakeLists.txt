# FordRuntime — standalone player (no editor UI), links FordEngine only.
add_executable(FordRuntime ${CMAKE_CURRENT_SOURCE_DIR}/../src/RuntimeApp/main.cpp)
target_link_libraries(FordRuntime PRIVATE FordEngine)

add_custom_command(TARGET FordRuntime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:FordRuntime>/Resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/Resources/FE.png $<TARGET_FILE_DIR:FordRuntime>/Resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/Resources/Angel wish.ttf" $<TARGET_FILE_DIR:FordRuntime>/Resources/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Resources/skull $<TARGET_FILE_DIR:FordRuntime>/Resources/skull
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Resources/Skeleton $<TARGET_FILE_DIR:FordRuntime>/Resources/Skeleton
)
