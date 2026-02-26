#ifndef _INCLUDE_BALL_TYPES_C_DEBUGBREAK_H_
#	define _INCLUDE_BALL_TYPES_C_DEBUGBREAK_H_

#include "platform.h"

#	if defined( BALL_MSVC )
#		define BALL_DEBUGBREAK() __debugbreak()
#	elif defined( BALL_GNUC )
#		define BALL_DEBUGBREAK_RAISE() raise( SIGTRAP )

#		if defined( __has_builtin )
#			if __has_builtin( __builtin_debugtrap )
#				define BALL_DEBUGBREAK() __builtin_debugtrap()
#			elif __has_builtin( __builtin_trap )
#				define BALL_DEBUGBREAK() __builtin_trap()
#			else
#				define BALL_DEBUGBREAK() BALL_DEBUGBREAK_RAISE()
#			endif
#		else
#			define BALL_DEBUGBREAK() BALL_DEBUGBREAK_RAISE()
#		endif
#	else
#		define BALL_DEBUGBREAK() BALL_DEBUGBREAK_RAISE()
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_DEBUGBREAK_H_ )
