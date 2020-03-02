project(tests)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})
set(ALL_TESTS "test_convert" "test_date" "test_fmt" "test_getopt" "test_goom_logging" "test_settings")

foreach(TEST ${ALL_TESTS}) 
	add_executable(${TEST}
		${TEST}.cpp
	)
	target_link_libraries(${TEST}
		PRIVATE
			libs::utils
	)
	target_compile_features(${TEST}
		PRIVATE
			${COMMON_CXX_COMPILE_FEATURES}
	)
	target_compile_options(${TEST} 
		PUBLIC 
			"$<$<CONFIG:DEBUG>:${COMMON_CXX_DEBUG_OPTIONS}>"
	)
	target_compile_options(${TEST}
		PUBLIC 
			"$<$<CONFIG:RELEASE>:${COMMON_CXX_RELEASE_OPTIONS}>"
	)
endforeach()

target_link_libraries(test_goom_logging
	PRIVATE
		libs::goom
		libs::utils
)

set_target_properties(${ALL_TESTS} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin
)
