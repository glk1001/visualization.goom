cmake_minimum_required(VERSION 3.13)
project(visualization.goom)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Weffc++ -Wextra -Wsign-conversion")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wsign-conversion")

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

include_directories(${includes}
                    ${GLM_INCLUDE_DIR}
                    ${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways (becomes done in future)
                    ${PROJECT_SOURCE_DIR}/lib
                    ${PROJECT_SOURCE_DIR}/lib/goom/src)

set(GOOM_SOURCES src/Main.cpp)

set(GOOM_HEADERS src/CircularBuffer.h
                 src/AudioData.h
                 src/Main.h)

list(APPEND DEPLIBS goom)

build_addon(visualization.goom GOOM DEPLIBS)

include(CPack)
