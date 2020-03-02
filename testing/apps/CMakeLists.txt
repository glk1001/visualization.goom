project(apps)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

set(ALL_APPS "test_goom" "show_goom_output")

message(STATUS "Apps compile features = \"${COMMON_CXX_COMPILE_FEATURES}\".")
message(STATUS "Apps C++ warnings = \"${COMMON_CXX_WARNINGS}\".")

add_executable(test_goom
	test_goom.cpp
	goom_tester.cpp
)
add_executable(show_goom_output
	show_goom_output.cpp
)

foreach(APP ${ALL_APPS}) 
	target_link_libraries(${APP}
		PRIVATE
			libs::goom
			libs::utils
			pthread
	)
	target_compile_features(${APP}
		PRIVATE
			${COMMON_CXX_COMPILE_FEATURES}
	)
	target_compile_options(${APP} 
		PRIVATE 
			${COMMON_CXX_WARNINGS}
			-pthread
	)
endforeach()

target_link_libraries(show_goom_output
	PRIVATE
		X11 
		m 
		GL 
		GLEW 
		glut
		libs::goom
		libs::utils
        pthread
)

set_target_properties(${ALL_APPS} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin
)
