cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0092)
	cmake_policy(SET CMP0092 NEW) # Don't add -W3 warning level by default.
endif()

project(ball
	VERSION 1.0.0
	DESCRIPTION "Ball C++ libraries - Types"
	HOMEPAGE_URL "https://github.com/Wend4r/cpp-sample"
	LANGUAGES C CXX
)

string(REGEX REPLACE "^[^-]*-" "" PROJECT_NAME_SUBSTRING "${PROJECT_NAME}")
string(TOLOWER "${PROJECT_NAME_SUBSTRING}" PROJECT_NAME_LOWER)
string(TOUPPER "${PROJECT_NAME_SUBSTRING}" PROJECT_NAME_UPPER)

option(BALL_ENABLE_ASSERT ON)
option(BALL_ENABLE_MODULES ON)

set(PROJECT_OUTPUT_NAME "ball-types")

set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SOURCES
	${SOURCE_DIR}/ball/types/memory.c
	${SOURCE_DIR}/ball/types/c/assert.cpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

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
	CXX_SCAN_FOR_MODULES ON

	OUTPUT_NAME ${PROJECT_OUTPUT_NAME}
)

if(BALL_ENABLE_MODULES)
	target_sources(${PROJECT_NAME}
	PUBLIC
		FILE_SET cxx_modules
		TYPE CXX_MODULES
		BASE_DIRS ${INCLUDE_DIR}
		FILES
			${INCLUDE_DIR}/ball/new.cppm
			${INCLUDE_DIR}/ball/types/base/arch.cppm
			${INCLUDE_DIR}/ball/types/base/characters.cppm
			${INCLUDE_DIR}/ball/types/base/fixed.cppm
			${INCLUDE_DIR}/ball/types/base.cppm
			${INCLUDE_DIR}/ball/types.cppm
	)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_ASSERT_NAME} ${PROJECT_MEMORY_NAME})

include(CTest)
include(cmake/ball/types/base/tests.cmake)
include(cmake/ball/types/tests.cmake)
