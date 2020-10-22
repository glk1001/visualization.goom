cmake_minimum_required(VERSION 3.13)
project(goom)

if(WIN32)
 add_definitions(-D_WIN32PC)
endif()
if ($ENV{DO_GOOM_LOGGING})
  ADD_DEFINITIONS(-DDO_GOOM_LOGGING)
  MESSAGE("Added 'DO_GOOM_LOGGING' #define.")
endif()

set (SRCE_FILES
                 src/colorutils.cpp
                 src/convolve_fx.cpp
                 src/draw_methods.cpp
                 src/filters.cpp
                 src/flying_stars_fx.cpp
                 src/gfontlib.cpp
                 src/gfontrle.cpp
                 src/goom_control.cpp
                 src/goom_dots_fx.cpp
                 src/goom_draw.cpp
                 src/goom_testing.cpp
                 src/goom_visual_fx.cpp
                 src/ifs_fx.cpp
                 src/lines_fx.cpp
                 src/sound_info.cpp
                 src/tentacles_fx.cpp
                 src/tentacle_driver.cpp
                 src/tentacles.cpp
)

add_library(${PROJECT_NAME} STATIC
                 ${SRCE_FILES}
)

add_library(libs::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/goom/include/goom
        ${CMAKE_SOURCE_DIR}/lib/goomutils/include
)
target_include_directories(${PROJECT_NAME}
    SYSTEM PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/vivid/include
        ${CMAKE_SOURCE_DIR}/lib/cereal/include
        ${CMAKE_SOURCE_DIR}/lib/fmt/include
        ${CMAKE_SOURCE_DIR}/lib/magic_enum/include
)

message(STATUS "Goom compile features = \"${COMMON_C_COMPILE_FEATURES}\".")
target_compile_features(${PROJECT_NAME}
    PRIVATE
        ${COMMON_C_COMPILE_FEATURES} 
)

message(STATUS "Goom C warnings = \"${COMMON_C_WARNINGS}\".")
target_compile_options(${PROJECT_NAME}
    PRIVATE 
        ${COMMON_C_WARNINGS}
)

set_property(TARGET goom PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET goom PROPERTY CXX_STANDARD 20)
