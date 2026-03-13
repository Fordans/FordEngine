# FordEditor - Editor executable
set(FordEditor_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../src/Editor/main.cpp)

if(WIN32)
    set(FE_ICO_PATH "${CMAKE_SOURCE_DIR}/Resources/FE.ico")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/FordEditor.rc.in ${CMAKE_CURRENT_BINARY_DIR}/FordEditor.rc @ONLY)
    list(APPEND FordEditor_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/FordEditor.rc)
endif()

add_executable(FordEditor ${FordEditor_SOURCES})
target_link_libraries(FordEditor PRIVATE FordEngine)

# Copy Resources to exe dir (icons, fonts, etc.)
add_custom_command(TARGET FordEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:FordEditor>/Resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/Resources/FE.png $<TARGET_FILE_DIR:FordEditor>/Resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/Resources/Angel wish.ttf" $<TARGET_FILE_DIR:FordEditor>/Resources/
)
