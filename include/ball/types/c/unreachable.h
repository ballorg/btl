#ifndef _INCLUDE_BALL_TYPES_C_UNREACHABLE_H_
#	define _INCLUDE_BALL_TYPES_C_UNREACHABLE_H_

#    include "platform.h"

#	if defined( BALL_MSVC )
#		define BALL_UNREACHABLE() __assume( 0 )
#	elif defined( BALL_GNUC )
#		ifdef __has_builtin
#			if __has_builtin( __builtin_unreachable )
#				define BALL_UNREACHABLE() __builtin_unreachable()
#			else
#				define BALL_UNREACHABLE() ( ( void )0 )
#			endif
#		else
#			define BALL_UNREACHABLE() ( ( void )0 )
#		endif
#	else
#		define BALL_UNREACHABLE() ( ( void )0 )
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_UNREACHABLE_H_ )
