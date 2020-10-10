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
                 src/drawmethods.cpp
                 src/filters.cpp
                 src/flying_stars_fx.cpp
                 src/gfontlib.cpp
                 src/gfontrle.cpp
                 src/goom_config_param.cpp
                 src/goom_core.cpp
                 src/goom_plugin_info.cpp
                 src/goom_testing.cpp
                 src/ifs.cpp
                 src/lines.cpp
                 src/sound_info.cpp
                 src/tentacle3d.cpp
                 src/tentacles_driver.cpp
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
        ${CMAKE_SOURCE_DIR}/lib/vivid/include
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
