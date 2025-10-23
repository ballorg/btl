#ifndef _INCLUDE_BALL_TYPES_C_MEMORY_H_
#	define _INCLUDE_BALL_TYPES_C_MEMORY_H_

#	include "macros.h"

#	ifdef _WIN32
BALL_DLL_IMPORT_C unsigned long long _msize( void *pMem );
#	else
BALL_DLL_IMPORT_C unsigned long long malloc_usable_size( void *pMem );
inline unsigned long long _msize( void *pMem ) { return malloc_usable_size( pMem ); }
#	endif

// BALL_DLL_IMPORT_C void *malloc( unsigned long long nSize );
// BALL_DLL_IMPORT_C void *realloc( void *pMem, unsigned long long nSize );
BALL_DLL_IMPORT_C void free( void *pMem );

// BALL_DLL_IMPORT_C int memcmp( const void *pLhs, const void *pRhs, unsigned long long nSize );
// BALL_DLL_IMPORT_C void *memcpy( void *pDest, const void *_Src, unsigned long long nSize );
// BALL_DLL_IMPORT_C void *memmove( void *pDest, const void *_Src, unsigned long long nSize );

#endif // !defined( _INCLUDE_BALL_TYPES_C_MEMORY_H_ )
