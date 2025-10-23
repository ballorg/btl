#ifndef _INCLUDE_BALL_TYPES_MEMORYALIGNED_H_
#	define _INCLUDE_BALL_TYPES_MEMORYALIGNED_H_

#	include "base/arch.h"
#	include "base/fixed.h"

#	if defined( _WIN32 )
#		include "c/memoryaligned.h"

inline ptr_t Ball_AllocAlign( size_t nSize, size_t nAlign ) { return _aligned_malloc( nSize, nAlign ); }
inline void Ball_FreeAlign( ptr_t pMem ) { return _aligned_free( pMem ); }
inline ptr_t Ball_ReallocAlign( ptr_t pMem, size_t nSize, size_t nAlign ) { return _aligned_realloc( pMem, nSize, nAlign ); }
inline size_t Ball_MemSize( ptr_t pMem, size_t nAlign, size_t nOffset ) { return _aligned_msize( pMem, nAlign, nOffset ); }
#	else // !defined( _WIN32 )
#		include "c/macros.h"

BALL_EXTERN_C ptr_t Ball_AllocAlign( size_t nSize, size_t nAlign );
BALL_EXTERN_C void Ball_FreeAlign( ptr_t pMem );
BALL_EXTERN_C ptr_t Ball_ReallocAlign( ptr_t pMem, size_t nSize, size_t nAlign );
BALL_EXTERN_C size_t Ball_MemSize( ptr_t pMem, size_t nAlign, size_t nOffset );
#	endif // defined( _WIN32 )

#endif // !defined( _INCLUDE_BALL_TYPES_MEMORYALIGNED_H_ )
