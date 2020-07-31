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
                 src/goom_testing.c
                 src/goom_tools.c
                 src/config_param.c
                 src/convolve_fx.c
                 src/filters.c
                 src/flying_stars_fx.c
                 src/gfontlib.c
                 src/gfontrle.c
                 src/goom_core.c
                 src/goom_logging.c
                 src/graphic.c
                 src/ifs.c
                 src/lines.c
                 src/mathtools.c
                 src/math_utils.cpp
                 src/SimplexNoise.cpp
                 src/sound_tester.c
                 src/surf3d.cpp
                 src/tentacle_driver.cpp
                 src/tentacle3d.cpp
                 src/tentacles_new.cpp
                 src/plugin_info.c
                 src/drawmethods.cpp
)

add_library(${PROJECT_NAME} STATIC
                 ${SRCE_FILES}
)

add_library(libs::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# How to include this?????
set(UTILS_INCLUDE_DIR "/home/greg/Prj/github/xbmc/visualization.goom.testing/libs/utils/include")
set(VIVID_INCLUDE_DIR "/home/greg/Prj/github/vivid/include")

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${UTILS_INCLUDE_DIR}
        ${VIVID_INCLUDE_DIR}      
        ${CMAKE_CURRENT_SOURCE_DIR}
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
#set_property(TARGET goom PROPERTY C_STANDARD 11)
set_property(TARGET goom PROPERTY CXX_STANDARD 20)
#set_property(TARGET goom PROPERTY LANGUAGE CXX)
set_source_files_properties(${SRCE_FILES} PROPERTIES LANGUAGE CXX )
