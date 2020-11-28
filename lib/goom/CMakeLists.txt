cmake_minimum_required(VERSION 3.13)

project(goom LANGUAGES CXX)

if(WIN32)
    add_definitions(-D_WIN32PC)
endif()
if ($ENV{DO_GOOM_LOGGING})
    ADD_DEFINITIONS(-DDO_GOOM_LOGGING)
    MESSAGE("Added 'DO_GOOM_LOGGING' #define.")
endif()

add_library(${PROJECT_NAME} 
    STATIC
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
        src/tentacle_driver.cpp
        src/tentacles.cpp
        src/tentacles_fx.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${LIBS_DIR}/goom/include
        ${LIBS_DIR}/goomutils/include
    PRIVATE
        ${LIBS_DIR}/goom/include/goom
)

target_include_directories(${PROJECT_NAME}
    SYSTEM PRIVATE
        ${LIBS_DIR}/vivid/include
        ${LIBS_DIR}/cereal/include
        ${LIBS_DIR}/fmt/include
        ${LIBS_DIR}/magic_enum/include
)

set_target_properties( ${PROJECT_NAME} 
    PROPERTIES 
        POSITION_INDEPENDENT_CODE ON
)

find_package(OpenMP REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)

