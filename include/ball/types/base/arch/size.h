#ifndef _INCLUDE_BALL_TYPES_BASE_ARCH_SIZE_H_
#	define _INCLUDE_BALL_TYPES_BASE_ARCH_SIZE_H_

#	if defined( BALL_SIZE_DEFINE_GLOBAL )
#		if !defined( _SIZE_T_DEFINED )
#			if defined( _MSC_VER )
#				if defined( _WIN64 )
typedef unsigned __int64 size_t;
#				else
typedef unsigned int size_t;
#				endif
#			elif defined( __SIZE_TYPE__ )
typedef __SIZE_TYPE__ size_t;
#			else
typedef long int size_t;
#			endif
#			define _SIZE_T_DEFINED
#		endif
#	elif defined( BALL_SIZE_IMPORT_OR_DEFINE_LOCAL )
#		if defined( _SIZE_T_DEFINED ) && defined( __cplusplus )
using ::size_t;
#		elif defined( _MSC_VER )
#			if defined( _WIN64 )
typedef unsigned __int64 size_t;
#			else
typedef unsigned int size_t;
#			endif
#		elif defined( __SIZE_TYPE__ )
typedef __SIZE_TYPE__ size_t;
#		else
typedef long int size_t;
#		endif
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_BASE_ARCH_SIZE_H_ )
