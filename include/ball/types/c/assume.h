#ifndef _INCLUDE_BALL_TYPES_C_ASSUME_H_
#	define _INCLUDE_BALL_TYPES_C_ASSUME_H_

// BALL_ASSUME(x)
//  - Optimization hint to the compiler that x is always true in Release.
//  - In GCC/Clang we route false-case into UB to enable more aggressive opts.
#	if defined( _MSC_VER )
#		define BALL_ASSUME( x ) __assume( x )
#	elif defined( __GNUC__ ) || defined( __clang__ )
#		define BALL_ASSUME( x ) do { if ( !(x) ) __builtin_unreachable(); } while ( 0 )
#	else
#		define BALL_ASSUME( x ) ( ( void )0 )
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_ASSUME_H_ )
