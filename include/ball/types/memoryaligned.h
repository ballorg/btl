#ifndef _INCLUDE_BALL_TYPES_MEMORYALIGNED_H_
#	define _INCLUDE_BALL_TYPES_MEMORYALIGNED_H_

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/macros.h"
#	include "memory.h"

///-----------------------------------------------------------------------------
/// @brief Aligned allocation over Ball_Alloc & Co., headerless as well: the size
///        and the alignment a block was allocated with are supplied back by the
///        caller on release/resize.
///-----------------------------------------------------------------------------
BALL_EXTERN_C ptr_t Ball_AllocAlign( size_t nSize, size_t nAlign );
BALL_EXTERN_C ptr_t Ball_ReallocAlign( ptr_t pMem, size_t nOldSize, size_t nNewSize, size_t nAlign );
BALL_EXTERN_C void Ball_FreeAlign( ptr_t pMem, size_t nSize, size_t nAlign );

#endif // !defined( _INCLUDE_BALL_TYPES_MEMORYALIGNED_H_ )
