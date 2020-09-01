project(goomtests)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

message(STATUS "Tests compile features = \"${COMMON_CXX_COMPILE_FEATURES}\".")
message(STATUS "Tests C++ warnings = \"${COMMON_CXX_WARNINGS}\".")

add_executable(testmain
    src/testmain.cpp
)

target_link_libraries(testmain
    PRIVATE
        goom
        goomutils
        vivid
        pthread
)

target_include_directories(testmain 
    PUBLIC 
        "/home/greg/Prj/github/catch2/include"
)

# Hacking?????????????????????????
set(PARENT_LIB_DIR "/home/greg/Prj/github/xbmc/visualization.goom/build/visualization.goom-prefix/src/visualization.goom-build/lib")
set(GOOM_LIB "${PARENT_LIB_DIR}/goom/libgoom.a")
set(GOOMUTILS_LIB "${PARENT_LIB_DIR}/goomutils/libgoomutils.a")
set(VIVID_LIB "${PARENT_LIB_DIR}/vivid/libvivid.a")

#add_library(goom STATIC IMPORTED)
#set_property(TARGET goom PROPERTY IMPORTED_LOCATION ${GOOM_LIB})

add_library(goomutils STATIC IMPORTED)
set_property(TARGET goomutils PROPERTY IMPORTED_LOCATION ${GOOMUTILS_LIB})

add_library(vivid STATIC IMPORTED)
set_property(TARGET vivid PROPERTY IMPORTED_LOCATION ${VIVID_LIB})

set_target_properties(testmain PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin
)
