#ifndef _INCLUDE_BALL_TYPES_CALLOCATOR_HPP_
#	define _INCLUDE_BALL_TYPES_CALLOCATOR_HPP_

#	pragma once

/// @brief Stateless byte-level allocation policy. The blocks carry no header, so
///        every release/resize takes back the size the block was allocated with.
class CAllocatorBase
{
public:
	static ptr_t Alloc( size_t nSize, size_t nAligned )
	{
		return Ball_AllocAlign( nSize, nAligned );
	}

	static ptr_t Realloc( ptr_t pMem, size_t nOldSize, size_t nNewSize, size_t nAligned )
	{
		return Ball_ReallocAlign( pMem, nOldSize, nNewSize, nAligned );
	}

	static void Free( ptr_t pMem, size_t nSize, size_t nAligned )
	{
		Ball_FreeAlign( pMem, nSize, nAligned );
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

	static T *Realloc( T *pMem, I nOldCount, I nNewCount, size_t nAligned )
	{
		return reinterpret_cast< T * >( Base_t::Realloc( pMem, nOldCount * sizeof( T ), nNewCount * sizeof( T ), nAligned ) );
	}
	static T *Realloc( T *pMem, I nOldCount, I nNewCount ) { return Realloc( pMem, nOldCount, nNewCount, ALIGN_SIZE ); }

	static void Free( T *pMem, I nCount, size_t nAligned )
	{
		Base_t::Free( pMem, nCount * sizeof( T ), nAligned );
	}
	static void Free( T *pMem, I nCount ) { Free( pMem, nCount, ALIGN_SIZE ); }
}; // class CAllocator

#endif // !defined( _INCLUDE_BALL_TYPES_CALLOCATOR_HPP_ )
