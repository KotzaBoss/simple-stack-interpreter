cmake_minimum_required(VERSION 3.25)

project(sage)

set(CMAKE_CXX_COMPILER "/usr/bin/g++")

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "Configuring ${CMAKE_BUILD_TYPE} build")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	message(CHECK_START "Finding ccache")
	find_program(CCACHE ccache)
	if (NOT ${CCACHE} MATCHES ".*NOTFOUND")
		message(CHECK_PASS ${CCACHE})
		set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
	else()
		message(CHECK_FAIL "Not found")
	endif()
endif()

include(CTest)
add_subdirectory(test)

