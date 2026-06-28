#ifndef _INCLUDE_BALL_TYPES_MEMORY_H_
#	define _INCLUDE_BALL_TYPES_MEMORY_H_

#	include "c/macros.h"

#	define BALL_SIZE_DEFINE_GLOBAL 1
#		include "base/arch/size.h"
#	undef BALL_SIZE_DEFINE_GLOBAL

#	if defined( BALL_MSVC )
BALL_DLL_IMPORT_RESTRICT_C void * __cdecl malloc( size_t nSize ) BALL_CRT_NOEXCEPT;
BALL_DLL_IMPORT_RESTRICT_C void * __cdecl realloc( void *pMem, size_t nSize ) BALL_CRT_NOEXCEPT;
BALL_DLL_IMPORT_C void __cdecl free( void *pMem ) BALL_CRT_NOEXCEPT;
BALL_DLL_IMPORT_C size_t __cdecl _msize( void *pMem ) BALL_CRT_NOEXCEPT;
BALL_EXTERN_C void * __cdecl memset( void *pDest, int nFill, size_t nCount ) BALL_CRT_NOEXCEPT;
BALL_EXTERN_C void * __cdecl memcpy( void *pDest, const void *pSrc, size_t nCount ) BALL_CRT_NOEXCEPT;
BALL_EXTERN_C void * __cdecl memmove( void *pDest, const void *pSrc, size_t nCount ) BALL_CRT_NOEXCEPT;
#	else
BALL_EXTERN_C void *malloc( size_t nSize ) BALL_CRT_NOEXCEPT;
BALL_EXTERN_C void *realloc( void *pMem, size_t nSize ) BALL_CRT_NOEXCEPT;
BALL_EXTERN_C void free( void *pMem ) BALL_CRT_NOEXCEPT;
BALL_EXTERN_C size_t malloc_usable_size( void *pMem ) BALL_CRT_NOEXCEPT;
inline size_t _msize( void *pMem ) { return malloc_usable_size( pMem ); }

BALL_EXTERN_C void *memset( void *pDest, int nFill, size_t nCount ) BALL_CRT_NOEXCEPT;
BALL_EXTERN_C void *memcpy( void *pDest, const void *pSrc, size_t nCount ) BALL_CRT_NOEXCEPT;
BALL_EXTERN_C void *memmove( void *pDest, const void *pSrc, size_t nCount ) BALL_CRT_NOEXCEPT;
#	endif // defined( BALL_MSVC )

#endif // !defined( _INCLUDE_BALL_TYPES_MEMORY_H_ )
