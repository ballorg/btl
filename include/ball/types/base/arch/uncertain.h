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

#	define BALL_SIZE_IMPORT_OR_DEFINE_LOCAL 1
#	include "size.h"
#	undef BALL_SIZE_IMPORT_OR_DEFINE_LOCAL

#endif // !defined( _INCLUDE_BALL_TYPES_BASE_ARCH_UNCERTAIN_H_ )
