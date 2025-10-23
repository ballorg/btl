#ifndef _INCLUDE_BALL_TYPES_C_MEMORYALIGNED_H_
#	define _INCLUDE_BALL_TYPES_C_MEMORYALIGNED_H_

#	include "macros.h"

BALL_DLL_IMPORT_C void *_aligned_malloc( unsigned long long nSize, unsigned long long nAlignment );
BALL_DLL_IMPORT_C void _aligned_free( void *p );
BALL_DLL_IMPORT_C unsigned long long _aligned_msize( void *p, unsigned long long nAlignment, unsigned long long nOffset );
BALL_DLL_IMPORT_C void *_aligned_realloc( void *p, unsigned long long nSize, unsigned long long nAlignment );

#endif // !defined( _INCLUDE_BALL_TYPES_C_MEMORYALIGNED_H_ )
