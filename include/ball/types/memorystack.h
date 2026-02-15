#ifndef _INCLUDE_BALL_TYPES_MEMORYSTACK_H_
#	define _INCLUDE_BALL_TYPES_MEMORYSTACK_H_

#	include "c/macros.h"

#	if defined( _MSC_VER )
BALL_EXTERN_C void *_alloca( size_t nSize );
#	endif

#	if defined( _MSC_VER )
#		define BALL_STACK_ALLOCA( nSize ) _alloca( nSize )
#	else // !defined( _MSC_VER )
#		define BALL_STACK_ALLOCA( nSize ) __builtin_alloca( nSize )
#	endif // defined( _MSC_VER )

#	define Ball_STACKALLOC_BEGIN( nSize ) BALL_STACK_ALLOCA( nSize )
#	define Ball_STACKALLOC_END( pMem )

#endif // !defined( _INCLUDE_BALL_TYPES_MEMORYSTACK_H_ )
