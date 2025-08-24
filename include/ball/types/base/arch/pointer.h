#ifndef _INCLUDE_BALL_TYPES_BASE_ARCH_POINTER_H_
#	define _INCLUDE_BALL_TYPES_BASE_ARCH_POINTER_H_

typedef void void_t;
typedef void_t *ptr_t;
#	if defined( __cplusplus ) && __cplusplus >= 201103L
typedef decltype( nullptr ) nullptr_t;
#	else // !(defined( __cplusplus ) && __cplusplus >= 201103L)
typedef void_t *nullptr_t;
#	endif // defined( __cplusplus ) && __cplusplus >= 201103L

typedef const char *cstr_t;

#endif // _INCLUDE_BALL_TYPES_BASE_ARCH_POINTER_H_
