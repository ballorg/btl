if(NOT BUILD_TESTING)
	return()
endif()

enable_testing()

set(PROJECT_TESTS_NAME ${PROJECT_NAME}-tests)
set(PROJECT_TESTS_OUTPUT_NAME ${PROJECT_OUTPUT_NAME}-tests)

add_executable(${PROJECT_TESTS_NAME}
	${SOURCE_DIR}/ball/types/tests/main.cpp
	${SOURCE_DIR}/ball/types/tests/case01_stlvector.cpp
	${SOURCE_DIR}/ball/types/tests/case02_vector.cpp
	${SOURCE_DIR}/ball/types/tests/case03_vectorsoa.cpp
	${SOURCE_DIR}/ball/types/tests/case04_vectorsoa_fixed/signed.cpp
	${SOURCE_DIR}/ball/types/tests/case04_vectorsoa_fixed/uncertain.cpp
	${SOURCE_DIR}/ball/types/tests/case04_vectorsoa_fixed/unsiged.cpp
	${SOURCE_DIR}/ball/types/tests/case04_vectorsoa_fixed.cpp
	${SOURCE_DIR}/ball/types/tests/case05_rbtree.cpp
	${SOURCE_DIR}/ball/types/tests/case06_reflection.cpp
	${SOURCE_DIR}/ball/types/tests/case07_hash.cpp
	${SOURCE_DIR}/ball/types/tests/case08_hashmap.cpp
	${SOURCE_DIR}/ball/types/tests/case10_delegate.cpp
	${SOURCE_DIR}/ball/types/tests/case11_mapbenchmark.cpp
)

target_sources(${PROJECT_TESTS_NAME}
PRIVATE
	FILE_SET cxx_modules
	TYPE CXX_MODULES
	BASE_DIRS ${SOURCE_DIR}/ball/types/tests
	FILES
		${SOURCE_DIR}/ball/types/tests/case01_stlvector.cppm
		${SOURCE_DIR}/ball/types/tests/case02_vector.cppm
		${SOURCE_DIR}/ball/types/tests/case03_vectorsoa.cppm
		${SOURCE_DIR}/ball/types/tests/case04_vectorsoa_fixed.cppm
		${SOURCE_DIR}/ball/types/tests/case05_rbtree.cppm
		${SOURCE_DIR}/ball/types/tests/case06_reflection.cppm
		${SOURCE_DIR}/ball/types/tests/case07_hash.cppm
		${SOURCE_DIR}/ball/types/tests/case08_hashmap.cppm
		${SOURCE_DIR}/ball/types/tests/case10_delegate.cppm
		${SOURCE_DIR}/ball/types/tests/case11_mapbenchmark.cppm
)

if(MSVC)
	target_compile_options(${PROJECT_TESTS_NAME} PRIVATE
		/W4
		/bigobj
		/GS
		/FS
		$<$<CONFIG:Debug>:/RTC1>
	)
endif()

target_link_libraries(${PROJECT_TESTS_NAME} PRIVATE ${PROJECT_NAME})

target_compile_definitions(${PROJECT_TESTS_NAME} PRIVATE BALL_TEST_ENABLE_MODULES=1)
set_target_properties(${PROJECT_TESTS_NAME} PROPERTIES
	CXX_SCAN_FOR_MODULES ON
)

set_target_properties(${PROJECT_TESTS_NAME} PROPERTIES
	OUTPUT_NAME ${PROJECT_TESTS_OUTPUT_NAME}

	CXX_EXTENSIONS OFF
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
)

add_test(
	NAME ${PROJECT_TESTS_NAME}
	COMMAND $<TARGET_FILE:${PROJECT_TESTS_NAME}>
)
