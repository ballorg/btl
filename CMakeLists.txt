cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0092)
	cmake_policy(SET CMP0092 NEW) # Don't add -W3 warning level by default.
endif()

project(ball
	VERSION 1.0.0
	DESCRIPTION "BTL"
	HOMEPAGE_URL "https://github.com/ballorg/btl"
	LANGUAGES C CXX
)

string(REGEX REPLACE "^[^-]*-" "" PROJECT_NAME_SUBSTRING "${PROJECT_NAME}")
string(TOLOWER "${PROJECT_NAME_SUBSTRING}" PROJECT_NAME_LOWER)
string(TOUPPER "${PROJECT_NAME_SUBSTRING}" PROJECT_NAME_UPPER)

option(BALL_ENABLE_ASSERT OFF)
option(BALL_ENABLE_MODULES ON)
option(BALL_ENABLE_TESTS OFF)

if(BALL_ENABLE_TESTS)
	set(BALL_ENABLE_MODULES ON CACHE BOOL "Build Ball C++20 modules" FORCE)
	set(BALL_TEST_ENABLE_MODULES ON CACHE BOOL "Build Ball tests through C++20 modules" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
	AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14
	AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15
)
	if(BALL_ENABLE_MODULES)
		message(WARNING "GCC 14 cannot reliably consume Ball's C++20 module BMIs; falling back to header mode")
		set(BALL_ENABLE_MODULES OFF CACHE BOOL "Build Ball C++20 modules" FORCE)
	endif()

	if(BALL_ENABLE_TESTS)
		message(WARNING "The current Ball test suite requires C++20 modules and is disabled for GCC 14")
		set(BALL_ENABLE_TESTS OFF CACHE BOOL "Build Ball tests" FORCE)
		set(BALL_TEST_ENABLE_MODULES OFF CACHE BOOL "Build Ball tests through C++20 modules" FORCE)
	endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	if(BALL_ENABLE_MODULES)
		message(WARNING "AppleClang does not provide CMake dependency scanning for C++20 modules; falling back to header mode")
		set(BALL_ENABLE_MODULES OFF CACHE BOOL "Build Ball C++20 modules" FORCE)
	endif()

	if(BALL_ENABLE_TESTS)
		message(WARNING "The current Ball test suite requires C++20 modules and is disabled for AppleClang")
		set(BALL_ENABLE_TESTS OFF CACHE BOOL "Build Ball tests" FORCE)
		set(BALL_TEST_ENABLE_MODULES OFF CACHE BOOL "Build Ball tests through C++20 modules" FORCE)
	endif()
endif()

if(BALL_ENABLE_MODULES AND CMAKE_VERSION VERSION_LESS 3.28)
	message(FATAL_ERROR "BALL_ENABLE_MODULES requires CMake 3.28 or newer")
endif()

set(PROJECT_OUTPUT_NAME "ball-types")

set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SOURCES
	${SOURCE_DIR}/ball/types/memory.c
	${SOURCE_DIR}/ball/types/time.c
	${SOURCE_DIR}/ball/types/c/assert.cpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_library(Ball::BTL ALIAS ${PROJECT_NAME})

if(BALL_ENABLE_ASSERT)
	target_compile_definitions(${PROJECT_NAME} PUBLIC BALL_ENABLE_ASSERT=1)
endif()

if(BALL_ENABLE_MODULES)
	target_compile_definitions(${PROJECT_NAME} PUBLIC BALL_ENABLE_MODULES=1)
endif()

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

set_target_properties(${PROJECT_NAME} PROPERTIES
	C_STANDARD 17
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS OFF

	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF

	OUTPUT_NAME ${PROJECT_OUTPUT_NAME}
)

if(BALL_ENABLE_MODULES)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		CXX_SCAN_FOR_MODULES ON
	)
	target_sources(${PROJECT_NAME}
	PUBLIC
		FILE_SET cxx_modules
		TYPE CXX_MODULES
		BASE_DIRS ${INCLUDE_DIR}
		FILES
			${INCLUDE_DIR}/ball/new.cppm
			${INCLUDE_DIR}/ball/time.cppm
			${INCLUDE_DIR}/ball/types/core.cppm
			${INCLUDE_DIR}/ball/types/bits.cppm
			${INCLUDE_DIR}/ball/types/elements.cppm
			${INCLUDE_DIR}/ball/types/fixed.cppm
			${INCLUDE_DIR}/ball/types/math.cppm
			${INCLUDE_DIR}/ball/types/number.cppm
			${INCLUDE_DIR}/ball/types/pair.cppm
			${INCLUDE_DIR}/ball/types/prefetch.cppm
			${INCLUDE_DIR}/ball/types/allocator.cppm
			${INCLUDE_DIR}/ball/types/array.cppm
			${INCLUDE_DIR}/ball/types/elementspack.cppm
			${INCLUDE_DIR}/ball/types/vectoriterator.cppm
			${INCLUDE_DIR}/ball/types/slotiterator.cppm
			${INCLUDE_DIR}/ball/types/reflect.cppm
			${INCLUDE_DIR}/ball/types/viewbase.cppm
			${INCLUDE_DIR}/ball/types/view.cppm
			${INCLUDE_DIR}/ball/types/stringview.cppm
			${INCLUDE_DIR}/ball/types/hash.cppm
			${INCLUDE_DIR}/ball/types/vector.cppm
			${INCLUDE_DIR}/ball/types/string.cppm
			${INCLUDE_DIR}/ball/types/rbtree.cppm
			${INCLUDE_DIR}/ball/types/hashmap.cppm
			${INCLUDE_DIR}/ball/types/delegate.cppm
			${INCLUDE_DIR}/ball/types.cppm
	)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_ASSERT_NAME} ${PROJECT_MEMORY_NAME})

if(BALL_ENABLE_TESTS)
	include(CTest)
	include(cmake/ball/types/base/tests.cmake)
	include(cmake/ball/types/tests.cmake)
endif()
