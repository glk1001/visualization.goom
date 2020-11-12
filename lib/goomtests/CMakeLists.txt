cmake_minimum_required(VERSION 3.13)

project(goomtests LANGUAGES CXX)

if(WIN32)
    add_definitions(-D_WIN32PC)
endif()

add_executable( ${PROJECT_NAME}
    src/test_main.cpp
    src/test_colorutils.cpp
    src/test_goomrand.cpp
    src/test_save_restore.cpp
    src/test_strutils.cpp
)

target_link_libraries( ${PROJECT_NAME}
    PUBLIC
        goom
        goomutils
        vivid
        pthread
)

target_include_directories( ${PROJECT_NAME} 
    PRIVATE 
        ${LIBS_DIR}/catch2/include
        ${LIBS_DIR}/goom/include/goom  
        ${LIBS_DIR}/goomutils/include/goomutils
)  
target_include_directories( ${PROJECT_NAME}
    SYSTEM PRIVATE 
        ${LIBS_DIR}/fmt/include
        ${LIBS_DIR}/cereal/include
        ${LIBS_DIR}/magic_enum/include
)

set_target_properties( ${PROJECT_NAME} 
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin
)
