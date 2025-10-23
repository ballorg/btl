#ifndef _INCLUDE_BALL_TYPES_C_DEBUGBREAK_H_
#	define _INCLUDE_BALL_TYPES_C_DEBUGBREAK_H_

#	if defined( _MSC_VER )
#		define BALL_DEBUGBREAK() __debugbreak()
#	elif defined( __GNUC__ ) || defined( __clang__ )
#		if defined( __has_builtin )
#			if __has_builtin( __builtin_debugtrap )
#				define BALL_DEBUGBREAK() __builtin_debugtrap()
#			else
#				define BALL_DEBUGBREAK() __builtin_trap()
#			endif
#		else
#			define BALL_DEBUGBREAK() __builtin_trap()
#		endif
#	else
#		define BALL_DEBUGBREAK() raise( SIGTRAP )
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_DEBUGBREAK_H_ )
