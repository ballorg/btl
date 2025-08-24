#ifndef _INCLUDE_BALL_TYPES_BASE_ARCH_UNSIGNED_H_
#	define _INCLUDE_BALL_TYPES_BASE_ARCH_UNSIGNED_H_

#	ifdef __cplusplus
typedef bool bool_t;
#	else // !defined( __cplusplus )
#		if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
typedef _Bool bool_t;
#		else // !(defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 199901L)
typedef unsigned char bool_t;
#		endif // defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 199901L
#	endif // defined( __cplusplus )

typedef unsigned char uchar_t;
typedef unsigned short int ushort_t;
typedef unsigned int uint_t;
typedef unsigned long int ulong_t;
typedef unsigned long long int ullong_t;
typedef unsigned long int usize_t;

#endif // _INCLUDE_BALL_TYPES_BASE_ARCH_UNSIGNED_H_
