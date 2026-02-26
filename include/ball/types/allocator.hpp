#ifndef _INCLUDE_BALL_TYPES_CALLOCATOR_HPP_
#	define _INCLUDE_BALL_TYPES_CALLOCATOR_HPP_

#	pragma once

#	include "base/arch/unsigned.h"
#	include "c/assert.h"
#	include "meta/fixed.hpp"
#	include "memoryaligned.h"

class CAllocatorBase
{
public:
	static void *Alloc( size_t nSize, size_t nAligned )
	{
		return Ball_AllocAlign( nSize, nAligned );
	}

	static void *Realloc( ptr_t pMem, size_t nSize, size_t nAligned )
	{
		return Ball_ReallocAlign( pMem, nSize, nAligned );
	}

	static void Free( ptr_t pMem )
	{
		Ball_FreeAlign( pMem );
	}

	static size_t Size( ptr_t pMem, size_t nAligned, size_t nOffset = 0 )
	{
		return Ball_SizeAlign( pMem, nAligned, nOffset );
	}
}; // class CAllocatorBase

template < typename I, typename T >
class CAllocator : public CAllocatorBase
{
public:
	using Base_t = CAllocatorBase;
	static constexpr size_t ALIGN_SIZE = alignof( T );

	static T *Alloc( I nCount, size_t nAligned )
	{
		return reinterpret_cast< T * >( Base_t::Alloc( nCount * sizeof( T ), nAligned ) );
	}
	static T *Alloc( I nCount ) { return Alloc( nCount, ALIGN_SIZE ); }

	static T *Realloc( T *pMem, I nCount, size_t nAligned )
	{
		return reinterpret_cast< T * >( Base_t::Realloc( pMem, nCount * sizeof( T ), nAligned ) );
	}
	static T *Realloc( T *pMem, I nCount ) { return Realloc( pMem, nCount, ALIGN_SIZE ); }
}; // class CAllocator

#endif // !defined( _INCLUDE_BALL_TYPES_CALLOCATOR_HPP_ )
