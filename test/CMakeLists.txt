set(TESTS interpreter.cpp)
set(RES interpreter.naive_factorial.txt)

foreach (test ${TESTS})
	string(REPLACE ".cpp" "" bin ${test})
	add_executable(${bin} ${test})
	
	target_include_directories(${bin}
		PRIVATE
			${PROJECT_SOURCE_DIR}/src
	)
	
	set_target_properties(${bin}
		PROPERTIES
			CXX_STANDARD			20
			CXX_STANDARD_REQUIRED	TRUE
			CXX_EXTENSIONS			TRUE
	)
	
	target_compile_options(${bin}
		PRIVATE
			-O2
			-Wall
			-Wextra
			-Wpedantic
			-fsanitize=undefined,address
	)

	target_compile_definitions(${bin}
		PRIVATE
		#			INTERPRETER_REPORT_EXECUTION
	)
	
	target_link_options(${bin}
		PRIVATE
		-fsanitize=undefined,address
	)

	foreach (f ${RES})
		add_custom_target(${bin}_program ALL
			COMMAND cp -f ${f} ${CMAKE_CURRENT_BINARY_DIR}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)
	endforeach()
	
	add_test(${bin} ${bin})
endforeach()
