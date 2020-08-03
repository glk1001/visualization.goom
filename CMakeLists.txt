cmake_minimum_required(VERSION 3.13)
project(visualization.goom)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(COMMON_C_WARNINGS -Wall -Wextra -Wsign-conversion)

find_package(Kodi REQUIRED)
find_package(glm REQUIRED)

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

add_subdirectory(lib/goom)


# Better way to do this??
set(VIVID_INCLUDE_DIR "/home/greg/Prj/github/vivid/include")
set(VIVID_LIB "/home/greg/Prj/github/vivid/build/libvivid.a")
set(UTILS_LIB "/home/greg/Prj/github/xbmc/visualization.goom.testing/build/libs/utils/libutils.a")


include_directories(${includes}
                    ${GLM_INCLUDE_DIR}
                    ${VIVID_INCLUDE_DIR}
                    ${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways (becomes done in future)
                    ${PROJECT_SOURCE_DIR}/lib
                    ${PROJECT_SOURCE_DIR}/lib/goom/src)

set(GOOM_SOURCES src/Main.cpp)

set(GOOM_HEADERS src/CircularBuffer.h
                 src/AudioData.h
                 src/Main.h)

if ($ENV{DO_TESTING})
  set (TESTING ON)
  include(../visualization.goom.testing/goom_testing)
endif()

list(APPEND DEPLIBS ${UTILS_LIB} goom ${UTILS_LIB} ${VIVID_LIB})

build_addon(visualization.goom GOOM DEPLIBS)

include(CPack)
