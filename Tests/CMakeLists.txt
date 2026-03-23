add_executable(fde_smoke_test fde_smoke_test.cpp)
target_link_libraries(fde_smoke_test PRIVATE FordEngine)

add_test(
    NAME fde_smoke_test
    COMMAND $<TARGET_FILE:fde_smoke_test>
    WORKING_DIRECTORY $<TARGET_FILE_DIR:fde_smoke_test>
)
