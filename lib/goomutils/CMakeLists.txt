cmake_minimum_required( VERSION 3.8 )
project(goomutils LANGUAGES CXX)

add_library( ${PROJECT_NAME} STATIC
    src/all_maps.cpp
    src/colormap.cpp
    src/goomrand.cpp
    src/logging.cpp
    src/mathutils.cpp
)

add_library(libs::vivid ALIAS ${PROJECT_NAME})

target_include_directories( ${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/goomutils/include/goomutils
        ${CMAKE_SOURCE_DIR}/lib/vivid/include
        ${CMAKE_SOURCE_DIR}/lib/fmt/include
)

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
