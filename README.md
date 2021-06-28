# yqueue

The multiple queues processor.

## how to build

### dependencies
1. [boost](https://www.boost.org/)
2. [CMake](https://cmake.org/) (build dependency)
3. [conan](https://conan.io/) (build dependency, optional)

### prepare to build
1. create and go to the build directory

        mkdir build && cd build

2. install the dependencies using conan (or you may skip this step if you already have boost installed)

        conan install .. --build=missing

3. configure the project (note that you can disable usage of conan if you want)

        cmake .. -DCMAKE_BUILD_TYPE=<Release|Debug> -DCMAKE_INSTALL_PREFIX=<path_to_install_library> -DUSE_CONAN=<ON|OFF>

### building
1. build the project

        cmake --build . --target all

2. install the library (copy files to <path_to_install_library>)

        cmake --build . --target install

### run unit tests

NOTE: you shoud enable tests with option "-DTESTS_ENABLE=ON" in configuration step

        cmake --build . --target test

## how to use

the library is header-only but you have to link the Boost.Fiber library and the thread library of the system with your app

### example for CMake projets

        find_package(Threads)
        find_package(Boost REQUIRED COMPONENTS fiber)

        find_package(yqueue 0.0.3 REQUIRED)

        add_executable(your-app main.cpp)
        target_link_libraries(your-app PRIVATE yqueue)
