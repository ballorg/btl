#ifndef _INCLUDE_BALL_TYPES_C_ASSUME_H_
#	define _INCLUDE_BALL_TYPES_C_ASSUME_H_

#   include "platform.h"

// BALL_ASSUME(x)
//  - Optimization hint to the compiler that x is always true in Release.
//  - In GCC/Clang we route false-case into UB to enable more aggressive opts.
#	if defined( BALL_MSVC )
#		define BALL_ASSUME( x ) __assume( x )
#	elif defined( BALL_GNUC )
#		define BALL_ASSUME( x ) do { if ( !(x) ) __builtin_unreachable(); } while ( 0 )
#	else // !defined( BALL_MSVC ) && !defined( BALL_GNUC )
#		define BALL_ASSUME( x ) ( ( void )0 )
#	endif // defined( BALL_MSVC ) || defined( BALL_GNUC )

#endif // !defined( _INCLUDE_BALL_TYPES_C_ASSUME_H_ )
