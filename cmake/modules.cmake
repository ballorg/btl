include(CMakeParseArguments)

function(ball_generate_public_module)
	cmake_parse_arguments(
		MODULE
		""
		"NAME;HEADER"
		"GLOBAL_HEADERS;EXPORT_IMPORTS"
		${ARGN}
	)

	if(NOT MODULE_NAME)
		message(FATAL_ERROR "ball_generate_public_module requires NAME")
	endif()
	if(MODULE_HEADER AND MODULE_EXPORT_IMPORTS)
		message(FATAL_ERROR "ball_generate_public_module(${MODULE_NAME}) accepts either HEADER or EXPORT_IMPORTS")
	endif()
	if(NOT MODULE_HEADER AND NOT MODULE_EXPORT_IMPORTS)
		message(FATAL_ERROR "ball_generate_public_module(${MODULE_NAME}) requires HEADER or EXPORT_IMPORTS")
	endif()

	set(BALL_PUBLIC_MODULE_GLOBAL_FRAGMENT "")
	set(BALL_PUBLIC_MODULE_CONTENT "")

	if(MODULE_GLOBAL_HEADERS)
		string(APPEND BALL_PUBLIC_MODULE_GLOBAL_FRAGMENT "module;\n\n")
		foreach(HEADER IN LISTS MODULE_GLOBAL_HEADERS)
			string(APPEND BALL_PUBLIC_MODULE_GLOBAL_FRAGMENT "#include <ball/${HEADER}>\n")
		endforeach()
		string(APPEND BALL_PUBLIC_MODULE_GLOBAL_FRAGMENT "\n")
	endif()

	if(MODULE_HEADER)
		string(APPEND BALL_PUBLIC_MODULE_CONTENT
			"#undef BALL_EXPORT\n"
			"#define BALL_EXPORT export\n"
			"#include \"ball/${MODULE_HEADER}\"\n"
		)
	else()
		foreach(IMPORT IN LISTS MODULE_EXPORT_IMPORTS)
			string(APPEND BALL_PUBLIC_MODULE_CONTENT "export import ${IMPORT};\n")
		endforeach()
	endif()
	string(REGEX REPLACE "\n$" "" BALL_PUBLIC_MODULE_CONTENT "${BALL_PUBLIC_MODULE_CONTENT}")

	string(REGEX REPLACE "^Ball\\." "" MODULE_FILENAME "${MODULE_NAME}")
	string(TOLOWER "${MODULE_FILENAME}" MODULE_FILENAME)
	set(MODULE_OUTPUT_DIR "${BALL_GENERATED_MODULE_DIR}/ball")
	file(MAKE_DIRECTORY "${MODULE_OUTPUT_DIR}")
	configure_file(
		"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ball/module.cppm.in"
		"${MODULE_OUTPUT_DIR}/${MODULE_FILENAME}.cppm"
		@ONLY
	)
endfunction()

function(ball_generate_module)
	cmake_parse_arguments(
		MODULE
		"NO_BALL_NEW;NO_DEFAULT_IMPORTS;NO_MEMORY"
		"PARTITION"
		"HEADERS;GLOBAL_HEADERS;IMPORTS"
		${ARGN}
	)

	if(NOT MODULE_PARTITION)
		message(FATAL_ERROR "ball_generate_module requires PARTITION")
	endif()
	if(NOT MODULE_HEADERS)
		message(FATAL_ERROR "ball_generate_module(${MODULE_PARTITION}) requires HEADERS")
	endif()

	set(BALL_MODULE_PARTITION "${MODULE_PARTITION}")
	set(BALL_MODULE_GLOBAL_INCLUDES "")
	set(BALL_MODULE_IMPORTS "")
	set(BALL_MODULE_INCLUDES "")

	if(NOT MODULE_NO_MEMORY)
		string(APPEND BALL_MODULE_GLOBAL_INCLUDES "#include <ball/types/memory.h>\n")
	endif()
	foreach(HEADER IN LISTS MODULE_GLOBAL_HEADERS)
		string(APPEND BALL_MODULE_GLOBAL_INCLUDES "#include <ball/types/${HEADER}>\n")
	endforeach()

	if(NOT MODULE_NO_BALL_NEW)
		string(APPEND BALL_MODULE_IMPORTS "import Ball.New;\n")
	endif()
	if(NOT MODULE_NO_DEFAULT_IMPORTS)
		string(APPEND BALL_MODULE_IMPORTS "import :Meta;\n")
	endif()
	foreach(IMPORT IN LISTS MODULE_IMPORTS)
		string(APPEND BALL_MODULE_IMPORTS "import ${IMPORT};\n")
	endforeach()

	foreach(HEADER IN LISTS MODULE_HEADERS)
		string(APPEND BALL_MODULE_INCLUDES "#include \"ball/types/${HEADER}\"\n")
	endforeach()

	string(TOLOWER "${MODULE_PARTITION}" MODULE_FILENAME)
	string(REPLACE "." "-" MODULE_FILENAME "${MODULE_FILENAME}")
	set(MODULE_OUTPUT_DIR "${BALL_GENERATED_MODULE_DIR}/ball/types")
	file(MAKE_DIRECTORY "${MODULE_OUTPUT_DIR}")
	configure_file(
		"${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ball/types/module.cppm.in"
		"${MODULE_OUTPUT_DIR}/${MODULE_FILENAME}.cppm"
		@ONLY
	)
endfunction()
