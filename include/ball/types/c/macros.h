#ifndef _INCLUDE_BALL_TYPES_C_MACROS_H_
#	define _INCLUDE_BALL_TYPES_C_MACROS_H_

#	include "platform.h"
#	include "constevaluated.h"
#	include "nouniqueaddress.h"

#	ifdef BALL_ENABLE_MODULE
#		define BALL_EXPORT export
#	else // !defined( BALL_ENABLE_MODULE )
#		define BALL_EXPORT
#	endif // defined( BALL_ENABLE_MODULE )

#	ifdef BALL_MSVC
#		define BALL_DLL_EXPORT __declspec( dllexport )
#		define BALL_DLL_IMPORT __declspec( dllimport )
#		define BALL_DLL_RESTRICT __declspec( restrict )
#	else // !defined( BALL_MSVC )
#		define BALL_DLL_EXPORT
#		define BALL_DLL_IMPORT
#		define BALL_DLL_RESTRICT
#	endif // defined( BALL_MSVC )

#	define BALL_DLL_EXPORT_C BALL_EXTERN_C BALL_DLL_EXPORT
#	define BALL_DLL_IMPORT_C BALL_EXTERN_C BALL_DLL_IMPORT
#	define BALL_DLL_EXPORT_RESTRICT_C BALL_EXTERN_C BALL_DLL_EXPORT BALL_DLL_RESTRICT
#	define BALL_DLL_IMPORT_RESTRICT_C BALL_EXTERN_C BALL_DLL_IMPORT BALL_DLL_RESTRICT

#	if defined( BALL_CXX ) && !defined( BALL_MSVC )
#		define BALL_CRT_NOEXCEPT noexcept
#	else
#		define BALL_CRT_NOEXCEPT
#	endif

#	if defined( BALL_MSVC )
#		define BALL_DLL_CONSTRUCTOR
#		define BALL_DLL_DESTRUCTATOR
#		define BALL_DLL_SECTION( name )
#	elif defined( BALL_GNUC ) // != defined( BALL_MSVC )
#		define BALL_DLL_CONSTRUCTOR __attribute__(( constructor ))
#		define BALL_DLL_DESTRUCTATOR __attribute__(( destructor ))
#		define BALL_DLL_SECTION( name ) __attribute__(( section( name ) ))
#	endif // defined( BALL_MSVC )

#	ifdef BALL_CXX
#		if !defined( BALL_MINGW ) && !defined( BALL_MSVC )
#			define BALL_NULL __null
#		else // !( !defined( BALL_MINGW ) && !defined( BALL_MSVC ) )
#			define BALL_NULL 0
#		endif // !defined( BALL_MINGW ) && !defined( BALL_MSVC )
#	else // !defined( BALL_CXX )
#		define BALL_NULL ( ( void * )0u )
#	endif // defined( BALL_CXX )

#	if defined( BALL_MSVC )
#		define BALL_WINAPI __stdcall
#	elif defined( BALL_GNUC )
#		define BALL_WINAPI __attribute__( ( stdcall ) )
#	else
#		define BALL_WINAPI
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_MACROS_H_ )
