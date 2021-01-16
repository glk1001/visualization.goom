cmake_minimum_required(VERSION 3.13)

project(visualization.goom)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMMON_C_WARNINGS -Wall -Wextra)

find_package(Kodi REQUIRED)
find_package(glm REQUIRED)

#find_package(Freetype REQUIRED)
set(FREETYPE2_DIR "/home/greg/Prj/github/freetype2")
set(FREETYPE_INCLUDE_DIRS "${FREETYPE2_DIR}/include")
set(FREETYPE_LIBRARIES ${FREETYPE2_DIR}/build/libfreetype.so)
message(STATUS "FREETYPE_INCLUDE_DIRS = \"${FREETYPE_INCLUDE_DIRS}\".")
message(STATUS "FREETYPE_LIBRARIES = \"${FREETYPE_LIBRARIES}\".")

if(APP_RENDER_SYSTEM STREQUAL "gl" OR NOT APP_RENDER_SYSTEM)
  find_package(OpenGl REQUIRED)
  set(DEPLIBS ${OPENGL_LIBRARIES})
  set(includes ${OPENGL_INCLUDE_DIR})
  add_definitions(${OPENGL_DEFINITIONS})
else()
  find_package(OpenGLES REQUIRED)
  set(DEPLIBS ${OPENGLES_LIBRARIES})
  set(includes ${OPENGLES_INCLUDE_DIR})
  add_definitions(${OPENGLES_DEFINITIONS})
endif()

set(LIBS_DIR ${CMAKE_SOURCE_DIR}/lib)
add_subdirectory(lib)

include_directories(${includes}
                    ${LIBS_DIR}/goom/include
                    ${LIBS_DIR}/goom/goomutils/include
                    ${LIBS_DIR}
                    SYSTEM ${GLM_INCLUDE_DIR}
                    SYSTEM ${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways (becomes done in future)
                    SYSTEM ${FREETYPE_INCLUDE_DIRS}
                    SYSTEM ${LIBS_DIR}/vivid/include
                    SYSTEM ${LIBS_DIR}/cereal/include
                    SYSTEM ${LIBS_DIR}/fmt/include)

set(GOOM_SOURCES src/Main.cpp)

set(GOOM_HEADERS src/CircularBuffer.h
                 src/Main.h)

if ($ENV{DO_TESTING})
  set (TESTING ON)
  include(../visualization.goom.testing/goom_testing)
endif()

list(APPEND DEPLIBS goom goomutils vivid)

add_compile_options(${CMAKE_COMMON_C_WARNINGS})

message(STATUS "C Compile features = \"${COMMON_C_COMPILE_FEATURES}\".")
message(STATUS "C warnings = \"${CMAKE_COMMON_C_WARNINGS}\".")
message(STATUS "C++ standard = \"${CMAKE_CXX_STANDARD}\".")
message(STATUS "C++ standard required = \"${CMAKE_CXX_STANDARD_REQUIRED}\".")
message(STATUS "C++ extensions = \"${CMAKE_CXX_EXTENSIONS}\".")

build_addon(${PROJECT_NAME} GOOM DEPLIBS)

include(CPack)
