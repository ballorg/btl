#ifndef _INCLUDE_BALL_TYPES_MEMORY_H_
#	define _INCLUDE_BALL_TYPES_MEMORY_H_

#	pragma once

#	include "base/arch.h"
#	include "c/memory.h"

///-----------------------------------------------------------------------------
/// @brief Allocate heap memory using CRT malloc.
///-----------------------------------------------------------------------------
static inline ptr_t Ball_Alloc( size_t nSize ) { return malloc( nSize ); }

///-----------------------------------------------------------------------------
/// @brief Realloc heap memory.
///-----------------------------------------------------------------------------
static inline ptr_t Ball_Realloc( ptr_t pMem, size_t nSize ) { return realloc( pMem, nSize ); }

///-----------------------------------------------------------------------------
/// @brief Free heap memory allocated.
///-----------------------------------------------------------------------------
static inline void Ball_Free( ptr_t pMem ) { free( pMem ); }

///-----------------------------------------------------------------------------
/// @brief Return usable size of heap block allocated.
///-----------------------------------------------------------------------------
static inline size_t Ball_Size( ptr_t pMem ) { return ( size_t )_msize( pMem ); }

#endif // !defined( _INCLUDE_BALL_TYPES_MEMORY_H_ )
