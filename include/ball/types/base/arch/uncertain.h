#ifndef _INCLUDE_BALL_TYPES_BASE_ARCH_UNCERTAIN_H_
#	define _INCLUDE_BALL_TYPES_BASE_ARCH_UNCERTAIN_H_

typedef char char_t;
typedef short int short_t;
typedef int int_t;
typedef long int long_t;
typedef long long int llong_t;
typedef float float_t;
typedef double double_t;
typedef long double ldouble_t;

#	if !defined( _SIZE_T_DEFINED ) && !( defined( _WIN32 ) && defined( _WIN64 ) )
#		ifdef __SIZE_TYPE__
typedef __SIZE_TYPE__ size_t;
#		else // !( !defined( _SIZE_T_DEFINED ) && !( defined( _WIN32 ) && defined( _WIN64 ) ) )
typedef long int size_t;
#		endif // !defined( __SIZE_TYPE__ )
#	endif // !defined( _SIZE_T_DEFINED ) && !( defined( _WIN32 ) && defined( _WIN64 ) )

#endif // _INCLUDE_BALL_TYPES_BASE_ARCH_UNCERTAIN_H_
