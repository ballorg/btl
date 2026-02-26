#ifndef _INCLUDE_BALL_TYPES_MEMORYSTACK_H_
#	define _INCLUDE_BALL_TYPES_MEMORYSTACK_H_

#	include "c/macros.h"

#	if defined( BALL_MSVC )
BALL_EXTERN_C void *_alloca( size_t nSize );
#	endif

#	if defined( BALL_MSVC )
#		define BALL_STACK_ALLOCA( nSize ) _alloca( nSize )
#	else // !defined( BALL_MSVC )
#		define BALL_STACK_ALLOCA( nSize ) __builtin_alloca( nSize )
#	endif // defined( BALL_MSVC )
#	define Ball_STACKALLOC_BEGIN( nSize ) BALL_STACK_ALLOCA( nSize )
#	define Ball_STACKALLOC_END( pMem )

#endif // !defined( _INCLUDE_BALL_TYPES_MEMORYSTACK_H_ )
