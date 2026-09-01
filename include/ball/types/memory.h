#ifndef _INCLUDE_BALL_TYPES_MEMORY_H_
#	define _INCLUDE_BALL_TYPES_MEMORY_H_

#	include "c/macros.h"

#	include "base/arch/pointer.h"
#	include "base/arch/size.h"

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

///-----------------------------------------------------------------------------
/// @brief Page-backed allocation (mmap/VirtualAlloc) with no header at all
///        (unlike Ball_AllocAlign & Co.): the mapping is handed out as is, so
///        the caller keeps the size and passes it back on release/resize.
///-----------------------------------------------------------------------------
BALL_EXTERN_C ptr_t Ball_Alloc( size_t nSize );
BALL_EXTERN_C ptr_t Ball_Realloc( ptr_t pMem, size_t nOldSize, size_t nNewSize );
BALL_EXTERN_C void Ball_Free( ptr_t pMem, size_t nSize );

#endif // !defined( _INCLUDE_BALL_TYPES_MEMORY_H_ )
