#ifndef _INCLUDE_BALL_TYPES_C_MATH_H_
#	define _INCLUDE_BALL_TYPES_C_MATH_H_

#	define BALL_MIN( a, b ) ( ( a < b ) ? a : b )
#	define BALL_MAX( a, b ) ( ( a > b ) ? a : b )

#	define BALL_IS_POW2( x ) ( ( x ) && ( ( ( x ) & ( ( x ) - 1 ) ) == 0 ) )
#	define BALL_ROUND_UP( v, g ) ( ( ( v ) + ( ( g ) - 1 ) ) & ~( ( g ) - 1) )
#	define BALL_ROUND_DOWN( v, g ) ( ( v ) & ~( ( g ) - 1) )

#endif // !defined( _INCLUDE_BALL_TYPES_C_MATH_H_ )
