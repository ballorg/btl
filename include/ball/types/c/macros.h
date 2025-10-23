#ifndef _INCLUDE_BALL_TYPES_C_MACROS_H_
#	define _INCLUDE_BALL_TYPES_C_MACROS_H_

#	ifdef __cplusplus
#		define BALL_LANGUAGE_STR "C++"
#		define BALL_EXTERN_C extern "C"
#		define BALL_EXTERN_CPP extern "C++"
#	else // !defined( __cplusplus )
#		define BALL_LANGUAGE_STR "C"
#		define BALL_EXTERN_C extern
#		define BALL_EXTERN_CPP extern
#	endif // defined( __cplusplus )

#	ifdef BALL_ENABLE_MODULE
#		define BALL_EXPORT export
#	else // !defined( BALL_ENABLE_MODULE )
#		define BALL_EXPORT
#	endif // defined( BALL_ENABLE_MODULE )

#	ifdef _MSC_VER
#		define BALL_DLL_EXPORT __declspec( dllexport )
#		define BALL_DLL_IMPORT __declspec( dllimport )
#	else // !defined( _MSC_VER )
#		define BALL_DLL_EXPORT
#		define BALL_DLL_IMPORT
#	endif // defined( _MSC_VER )

#	define BALL_DLL_EXPORT_C BALL_EXTERN_C BALL_DLL_EXPORT
#	define BALL_DLL_IMPORT_C BALL_EXTERN_C BALL_DLL_IMPORT

#	ifdef __cplusplus
#		if !defined( __MINGW32__ ) && !defined( _MSC_VER )
#			define BALL_NULL __null
#		else // !( !defined( __MINGW32__ ) && !defined( _MSC_VER ) )
#			define BALL_NULL 0
#		endif // !defined( __MINGW32__ ) && !defined( _MSC_VER )
#	else
#		define BALL_NULL ( ( void * )0u )
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_MACROS_H_ )
