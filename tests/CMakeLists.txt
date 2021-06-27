function(add_test_executable TEST_FILE)
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_FILE})
    target_compile_definitions(${TEST_NAME} PRIVATE TEST_SUITE="${TEST_NAME}")
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/YQueue)
    target_link_libraries(${TEST_NAME} PRIVATE ${PROJECT_NAME})

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_test_executable(HashMapTests.cpp)
add_test_executable(ProcessorTests.cpp)
add_test_executable(TypeTraitsTests.cpp)
add_test_executable(QueueTests.cpp)
add_test_executable(UtilsTests.cpp)
