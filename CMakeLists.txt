cmake_minimum_required(VERSION 3.8)
project(MyVariant)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp
        include/variant.hpp)

add_subdirectory(lib/googletest-master)
include_directories(lib/googletest-master/googletest/include)
include_directories(lib/googletest-master/googlemock/include)

add_executable(MyVariant_exe ${SOURCE_FILES})
add_library(MyVariant_lib SHARED ${SOURCE_FILES})
target_link_libraries(MyVariant_exe gtest gtest_main)

add_subdirectory(test)