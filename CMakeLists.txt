cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0092)
	cmake_policy(SET CMP0092 NEW) # Don't add -W3 warning level by default.
endif()

project(ball-types
	VERSION 1.0.0
	DESCRIPTION "Ball C++ libraries - Types"
	HOMEPAGE_URL "https://github.com/Wend4r/cpp-sample"
	LANGUAGES C CXX
)

string(REGEX REPLACE "^[^-]*-" "" PROJECT_NAME_SUBSTRING "${PROJECT_NAME}")
string(TOLOWER "${PROJECT_NAME_SUBSTRING}" PROJECT_NAME_LOWER)
string(TOUPPER "${PROJECT_NAME_SUBSTRING}" PROJECT_NAME_UPPER)

set(PROJECT_OUTPUT_NAME "ball-types")

set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
PUBLIC
	FILE_SET cxx_modules
	TYPE CXX_MODULES
	BASE_DIRS ${INCLUDE_DIR}
	FILES
		${INCLUDE_DIR}/ball/types/base/arch.cppm
		${INCLUDE_DIR}/ball/types/base/characters.cppm
		${INCLUDE_DIR}/ball/types/base/fixed.cppm
		${INCLUDE_DIR}/ball/types/base.cppm
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES
	C_STANDARD 20
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS OFF

	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF

	OUTPUT_NAME ${PROJECT_OUTPUT_NAME}
)
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})

include(cmake/base-tests.cmake)
