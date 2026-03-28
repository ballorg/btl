if(NOT BUILD_TESTING)
	return()
endif()

enable_testing()

set(PROJECT_TESTS_NAME ${PROJECT_NAME}-tests)
set(PROJECT_BASE_TESTS_OUTPUT_NAME ${PROJECT_OUTPUT_NAME}-tests)

add_executable(${PROJECT_TESTS_NAME}
	${SOURCE_DIR}/ball/types/tests/main.cpp
	${SOURCE_DIR}/ball/types/tests/case01_stlvector.cpp
	${SOURCE_DIR}/ball/types/tests/case02_vector.cpp
	${SOURCE_DIR}/ball/types/tests/case03_multivector.cpp
	${SOURCE_DIR}/ball/types/tests/case04_multivector_fixed/signed.cpp
	${SOURCE_DIR}/ball/types/tests/case04_multivector_fixed/uncertain.cpp
	${SOURCE_DIR}/ball/types/tests/case04_multivector_fixed/unsiged.cpp
	${SOURCE_DIR}/ball/types/tests/case04_multivector_fixed.cpp
	${SOURCE_DIR}/ball/types/tests/case10_delegate.cpp
)

target_link_libraries(${PROJECT_TESTS_NAME} PRIVATE ${PROJECT_NAME})

set_target_properties(${PROJECT_TESTS_NAME} PROPERTIES
	OUTPUT_NAME ${PROJECT_BASE_TESTS_OUTPUT_NAME}

	CXX_EXTENSIONS OFF
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
)

if(BALL_ENABLE_MODULES)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		CXX_SCAN_FOR_MODULES ON
	)
endif()

add_test(
	NAME ${PROJECT_TESTS_NAME}
	COMMAND $<TARGET_FILE:${PROJECT_TESTS_NAME}>
)
