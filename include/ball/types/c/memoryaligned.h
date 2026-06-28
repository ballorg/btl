#ifndef _INCLUDE_BALL_TYPES_C_MEMORYALIGNED_H_
#	define _INCLUDE_BALL_TYPES_C_MEMORYALIGNED_H_

#	include "macros.h"

#	if defined( BALL_MSVC )
BALL_DLL_IMPORT_RESTRICT_C void * __cdecl _aligned_malloc( unsigned __int64 nSize, unsigned __int64 nAlignment );
BALL_DLL_IMPORT_C void __cdecl _aligned_free( void *p );
BALL_DLL_IMPORT_C unsigned __int64 __cdecl _aligned_msize( void *p, unsigned __int64 nAlignment, unsigned __int64 nOffset );
BALL_DLL_IMPORT_RESTRICT_C void * __cdecl _aligned_realloc( void *p, unsigned __int64 nSize, unsigned __int64 nAlignment );
#	else
BALL_EXTERN_C void *_aligned_malloc( unsigned long long nSize, unsigned long long nAlignment );
BALL_EXTERN_C void _aligned_free( void *p );
BALL_EXTERN_C unsigned long long _aligned_msize( void *p, unsigned long long nAlignment, unsigned long long nOffset );
BALL_EXTERN_C void *_aligned_realloc( void *p, unsigned long long nSize, unsigned long long nAlignment );
#	endif // defined( BALL_MSVC )

#endif // !defined( _INCLUDE_BALL_TYPES_C_MEMORYALIGNED_H_ )
