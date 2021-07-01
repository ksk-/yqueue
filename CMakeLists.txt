cmake_minimum_required(VERSION 3.18)

project(yqueue CXX)

set(VERSION 0.0.4)

include(GNUInstallDirs)

option(USE_CONAN "Use conan for dependency find dependencies" ON)
option(TESTS_ENABLE "Enable unit tests" ON)

if(USE_CONAN)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
    set(Boost_INSTALL_DIR ${CONAN_BOOST_ROOT})
endif()

find_package(Threads)
find_package(Boost REQUIRED COMPONENTS fiber)

add_library(${PROJECT_NAME} INTERFACE)

target_compile_definitions(${PROJECT_NAME}
    INTERFACE
        YQUEUE_HASHMAP_BUCKETS=100
        YQUEUE_PROCESSOR_THREADS=0
        YQUEUE_QUEUE_CAPACITY=1000
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        Threads::Threads
        Boost::fiber
)

install(TARGETS ${PROJECT_NAME} DESTINATION cmake EXPORT ${PROJECT_NAME}-targets)
install(EXPORT ${PROJECT_NAME}-targets DESTINATION cmake)
install(DIRECTORY src/YQueue DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION cmake
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    VERSION ${VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION cmake
)

if(TESTS_ENABLE)
    enable_testing()
    add_subdirectory(tests)
endif()
