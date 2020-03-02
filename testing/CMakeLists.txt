cmake_minimum_required(VERSION 3.13)

project(goom-testing)

set(CMAKE_CXX_EXTENSIONS OFF)
set(COMMON_CXX_COMPILE_FEATURES cxx_std_20)
set(COMMON_CXX_WARNINGS -Wall -Weffc++ -Wextra -Wsign-conversion)

set(CMAKE_C_EXTENSIONS OFF)
set(COMMON_C_COMPILE_FEATURES c_std_11)
set(COMMON_C_WARNINGS -Wall -Wextra -Wsign-conversion)

enable_testing()

add_subdirectory(libs)
add_subdirectory(apps)
add_subdirectory(tests)

message(STATUS "Building with build type \"${CMAKE_BUILD_TYPE}\".")
