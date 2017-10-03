cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(RunAllTests
        MyVariantTest.cpp)

target_link_libraries(RunAllTests gtest gtest_main)
target_link_libraries(RunAllTests MyVariant_lib)