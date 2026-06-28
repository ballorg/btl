#ifndef _INCLUDE_BALL_TYPES_BASE_ARCH_POINTER_H_
#	define _INCLUDE_BALL_TYPES_BASE_ARCH_POINTER_H_

typedef void void_t;
typedef void_t *ptr_t;

#	if defined( __cplusplus ) && __cplusplus >= 201103L
#		if !defined( _NULLPTR_T_DEFINED )
typedef decltype( nullptr ) nullptr_t;
#			if defined( _MSC_VER )
#				define _NULLPTR_T_DEFINED
#			endif
#		endif
#	else // !( defined( __cplusplus ) && __cplusplus >= 201103L )
typedef void_t *nullptr_t;
#	endif // defined( __cplusplus ) && __cplusplus >= 201103L

typedef char *str_t;

#endif // !defined( _INCLUDE_BALL_TYPES_BASE_ARCH_POINTER_H_ )
