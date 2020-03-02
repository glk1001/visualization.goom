project(utils)

add_library(${PROJECT_NAME} STATIC
	CommandLineOptions.cpp
	goom_loggers.cpp
	load_save_settings.cpp
	test_common.cpp
	test_utils.cpp
	format.cc
	os.cc
)

add_library(libs::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		libs::goom
)

message(STATUS "Utils compile features = \"${COMMON_CXX_COMPILE_FEATURES}\".")
target_compile_features(${PROJECT_NAME}
	PRIVATE
		${COMMON_CXX_COMPILE_FEATURES}
)

message(STATUS "Utils C++ warnings = \"${COMMON_CXX_WARNINGS}\".")
target_compile_options(${PROJECT_NAME}
    PRIVATE 
		${COMMON_CXX_WARNINGS}
)

# add_subdirectory(tests)
