#ifndef _INCLUDE_BALL_TYPES_C_UNREACHABLE_H_
#	define _INCLUDE_BALL_TYPES_C_UNREACHABLE_H_

#	if defined( _MSC_VER )
#		define BALL_UNREACHABLE() __assume( 0 )
#	elif defined( __GNUC__ ) || defined( __clang__ )
#		define BALL_UNREACHABLE() __builtin_unreachable()
#	else
#		define BALL_UNREACHABLE() ( ( void )0 )
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_UNREACHABLE_H_ )
