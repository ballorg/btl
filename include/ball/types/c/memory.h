#ifndef _INCLUDE_BALL_TYPES_C_MEMORY_H_
#	define _INCLUDE_BALL_TYPES_C_MEMORY_H_

#	include "macros.h"

#	if defined( BALL_MSVC )
BALL_EXTERN_C unsigned long long __cdecl _msize( void *pMem );
BALL_EXTERN_C void * __cdecl memset( void *pDest, int nFill, size_t nCount );
BALL_EXTERN_C void * __cdecl memcpy( void *pDest, const void *pSrc, size_t nCount );
BALL_EXTERN_C void * __cdecl memmove( void *pDest, const void *pSrc, size_t nCount );
#	else
BALL_EXTERN_C unsigned long long malloc_usable_size( void *pMem );
inline unsigned long long _msize( void *pMem ) { return malloc_usable_size( pMem ); }

BALL_EXTERN_C void *memset( void *pDest, int nFill, size_t nCount );
BALL_EXTERN_C void *memcpy( void *pDest, const void *pSrc, size_t nCount );
BALL_EXTERN_C void *memmove( void *pDest, const void *pSrc, size_t nCount );
#	endif // defined( BALL_MSVC )

#endif // !defined( _INCLUDE_BALL_TYPES_C_MEMORY_H_ )
