#ifndef _INCLUDE_BALL_TYPES_MEMORYALIGNED_H_
#	define _INCLUDE_BALL_TYPES_MEMORYALIGNED_H_

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/memory.h"

#	include "c/macros.h"

BALL_EXTERN_C ptr_t Ball_AllocAlign( size_t nSize, size_t nAlign );
BALL_EXTERN_C void Ball_FreeAlign( ptr_t pMem );
BALL_EXTERN_C ptr_t Ball_ReallocAlign( ptr_t pMem, size_t nSize, size_t nAlign );
BALL_EXTERN_C size_t Ball_SizeAlign( ptr_t pMem, size_t nAlign, size_t nOffset );

#endif // !defined( _INCLUDE_BALL_TYPES_MEMORYALIGNED_H_ )
