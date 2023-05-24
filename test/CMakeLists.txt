set(TESTS
	main.test.cpp
	interpreter.test.cpp
	)
foreach (test ${TESTS})
	string(REPLACE ".test.cpp" "" bin ${test})
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
	
	target_link_options(${bin}
		PRIVATE
		-fsanitize=undefined,address
	)
	
	add_test(${bin} ${bin})
endforeach()
