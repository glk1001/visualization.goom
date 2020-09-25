project(goomtests)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

message(STATUS "Tests compile features = \"${COMMON_CXX_COMPILE_FEATURES}\".")
message(STATUS "Tests C++ warnings = \"${COMMON_CXX_WARNINGS}\".")

add_executable(test_main
    src/test_main.cpp
    src/test_colorutils.cpp
)

target_link_libraries(test_main
    PRIVATE
        goom
        goomutils
        vivid
        pthread
)

target_include_directories(test_main 
    PUBLIC 
        "/home/greg/Prj/github/catch2/include"
        "/home/greg/Prj/github/xbmc/visualization.goom/lib/goom/include/goom"  
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

set_target_properties(test_main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin
)
