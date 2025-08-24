#ifndef _INCLUDE_BALL_TYPES_BASE_CHARACTERS_H_
#	define _INCLUDE_BALL_TYPES_BASE_CHARACTERS_H_

#	include "fixed.h"

#	ifndef __cpp_char8_t
typedef uint8_t char8_t;
#	endif // !defined( __cpp_char8_t )

#	ifndef __cpp_unicode_characters
typedef uint16_t char16_t;
typedef uint32_t char32_t;
#	endif // !defined( __cpp_unicode_characters )

#	ifndef _WCHAR_T_DEFINED
#		ifdef __WCHAR_TYPE__
#			ifndef __cplusplus
typedef __WCHAR_TYPE__ wchar_t;
#			endif
#		else
#			if defined( _WIN32 ) || defined( _WIN64 )
typedef unsigned int wchar_t;
#			else // !(defined( _WIN32 ) || defined( _WIN64 ))
typedef unsigned int wchar_t;
#			endif // defined( _WIN32 ) || defined( _WIN64 )
#		endif // !defined( __WCHAR_TYPE__ )

#	endif // !defined( _WCHAR_T_DEFINED )

#endif // _INCLUDE_BALL_TYPES_BASE_CHARACTERS_H_
