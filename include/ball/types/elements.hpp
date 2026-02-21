#ifndef _INCLUDE_BALL_TYPES_ELEMENTS_HPP_
#	define _INCLUDE_BALL_TYPES_ELEMENTS_HPP_

#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/assert.h"
#	include "c/memory.h"
#	include "meta/traits.hpp"
#	include "meta/removereference.hpp"
#	include "xvalue.hpp"

template < typename T >
constexpr T *ConstructElement( T *pMemory )
{
	BALL_ASSERT( pMemory != nullptr );

	return reinterpret_cast< T * >( ::new( static_cast< void * >( pMemory ) ) T );
}

template < typename T, typename ...Ts >
constexpr T *ConstructElement( T *pMemory, Ts &&...args )
{
	BALL_ASSERT( pMemory != nullptr );

	return reinterpret_cast< T * >( ::new( static_cast< void * >( pMemory ) ) T( Forward< Ts >( args )... ) );
}

template < typename T >
constexpr void ConstructElements( T *pElement, const T *pEnd ) noexcept
{
	BALL_ASSERT_MESSAGE( pElement <= pEnd, "Invalid range" );

	while( pElement < pEnd )
	{
		ConstructElement( pElement );
		pElement++;
	}
}

template < typename T >
constexpr void DestructElement( T *pMemory )
{
	BALL_ASSERT( pMemory != nullptr );

	pMemory->~T();
}

template < typename I, typename T, I N >
constexpr void DestructElements( T ( *pMemory )[ N ] )
{
	BALL_ASSERT( pMemory != nullptr || N == I( 0 ) );

	for ( I n = 0; n < N; ++n )
	{
		DestructElement( pMemory[ n ] );
	}
}

template < typename T >
constexpr void DestructElements( T *pElement, const T *pEnd ) noexcept
{
	BALL_ASSERT_MESSAGE( pElement <= pEnd, "Invalid range in DestructElements" );

	while( pElement < pEnd )
	{
		DestructElement( pElement );
		pElement++;
	}
}

template < typename I, typename T >
constexpr T *CopyElements_Unified( const I nCount, T *pDest, const T *pSrc ) noexcept
{
	for ( I n = 0; n < nCount; ++n )
		pDest[ n ] = pSrc[ n ];

	return pDest;
}

///-----------------------------------------------------------------------------
/// @brief Copy raw elements from [pSrc, pSrcEnd) into destination starting at pDest.
///        Overlap-safe, memmove-like (byte-wise for trivially copyable types).
/// @return pDest
///-----------------------------------------------------------------------------
template < typename I, typename T >
constexpr T *CopyElements( const I nCount, T *pDest, const T *pSrc ) noexcept
{
	BALL_ASSERT( I( 0 ) <= nCount );
	BALL_ASSERT_MESSAGE( nCount == I( 0 ) || ( pDest != nullptr && pSrc != nullptr ), "Empty elements" );
	BALL_ASSERT_MESSAGE( nCount == I( 0 ) || pDest <= pSrc || pSrc + nCount <= pDest, "Unsafe overlap for forward copy" );

	return static_cast< T * >( memcpy( pDest, pSrc, static_cast< size_t >( nCount ) * sizeof( T ) ) );
}

template < typename I, typename T >
constexpr T *CopyElementsFromEnd_Unified( I nCount, T *pDest, const T *pSrc ) noexcept
{
	for ( I n = nCount; n-- > 0; )
		pDest[ n ] = pSrc[ n ];

	return pDest;
}

///-----------------------------------------------------------------------------
/// @brief Copy elements right-to-left: copy nLength elements from pSrc to pDest,
///        starting at the end (index nLength-1 down to 0).
///        Useful when ranges may overlap with pDest >= pSrc.
/// @return pDest
///-----------------------------------------------------------------------------
template < typename I, typename T >
constexpr T *CopyElementsFromEnd( const I nCount, T *pDest, const T *pSrc ) noexcept
{
	BALL_ASSERT( I( 0 ) <= nCount );
	BALL_ASSERT_MESSAGE( nCount == I( 0 ) || ( pDest != nullptr && pSrc != nullptr ), "Empty elements" );
	BALL_ASSERT_MESSAGE( nCount == I( 0 ) || pDest >= pSrc || pDest + nCount <= pSrc, "Unsafe overlap for backward copy" );

	return static_cast< T * >( memmove( pDest, pSrc, static_cast< size_t >( nCount ) * sizeof( T ) ) );
}

///-----------------------------------------------------------------------------
/// @brief Shift range [pSrc, pSrcEnd) right by exactly one element into @p pDest.
///        Forward algorithm (does not copy from end), overlap-safe for pDest==pSrc+1.
/// @return pDest
///-----------------------------------------------------------------------------
template < typename T >
constexpr T *ShiftElementsRight_ByOne( T *pDest, const T *pSrc, const T *pSrcEnd ) noexcept
{
	BALL_ASSERT_MESSAGE( pSrc <= pSrcEnd, "Invalid source range" );
	BALL_ASSERT_MESSAGE( pDest == pSrc + 1, "ShiftElementsRightByOne expects +1 shift" );

	using Diff_t = decltype( pSrcEnd - pSrc );
	const Diff_t nCount = pSrcEnd - pSrc;

	if ( nCount <= Diff_t( 0 ) )
		return pDest;

	T carry = pSrc[ 0 ];

	for ( Diff_t i = Diff_t( 1 ); i < nCount; ++i )
	{
		const T next = pSrc[ i ];

		pDest[ i - 1 ] = carry;
		carry = next;
	}

	pDest[ nCount - 1 ] = carry;

	return pDest;
}

///-----------------------------------------------------------------------------
/// @brief Shift elements in range [pSrc, pSrcEnd) to the left into @p pDest.
///        Copies left-to-right.
/// @return pDest
///-----------------------------------------------------------------------------
template < typename T >
constexpr T *ShiftElementsLeft( T *pDest, const T *pSrc, const T *pSrcEnd ) noexcept
{
	BALL_ASSERT_MESSAGE( pSrc <= pSrcEnd, "Invalid source range" );
	BALL_ASSERT_MESSAGE( pDest <= pSrc, "Expects destination at/before source" );

	using Diff_t = decltype( pSrcEnd - pSrc );

	const Diff_t nCount = pSrcEnd - pSrc;

	if ( nCount <= Diff_t( 0 ) || pDest == pSrc )
		return pDest;

	return CopyElements( nCount, pDest, pSrc );
}

template < typename T >
constexpr T *ShiftElementsRight_Unified( T *pDest, const T *pSrc, const T *pSrcEnd ) noexcept
{
	using Diff_t = decltype( pSrcEnd - pSrc );

	const Diff_t nCount = pSrcEnd - pSrc;
	const Diff_t nShift = pDest - pSrc;

	if ( nCount <= Diff_t( 0 ) || nShift <= Diff_t( 0 ) )
		return pDest;

	// Non-overlapping right move: regular forward copy is safe.
	if ( nShift >= nCount )
		return CopyElements_Unified( nCount, pDest, pSrc );

	// Common insert case: shift by one without reverse copy.
	if ( nShift == Diff_t( 1 ) )
		return ShiftElementsRight_ByOne( pDest, pSrc, pSrcEnd );

	return CopyElementsFromEnd_Unified( nCount, pDest, pSrc );
}

///-----------------------------------------------------------------------------
/// @brief Shift elements in range [pSrc, pSrcEnd) to the right into @p pDest.
///        Copies right-to-left.
/// @return pDest
///-----------------------------------------------------------------------------
template < typename T >
constexpr T *ShiftElementsRight( T *pDest, const T *pSrc, const T *pSrcEnd ) noexcept
{
	BALL_ASSERT_MESSAGE( pSrc <= pSrcEnd, "Invalid source range" );
	BALL_ASSERT_MESSAGE( pDest >= pSrc, "Expects destination at/after source" );

	using Diff_t = decltype( pSrcEnd - pSrc );

	const Diff_t nCount = pSrcEnd - pSrc;
	const Diff_t nShift = pDest - pSrc;

	if ( nCount <= Diff_t( 0 ) || nShift <= Diff_t( 0 ) )
		return pDest;

	return static_cast< T *>( memmove( pDest, pSrc, static_cast< size_t >( nCount ) * sizeof( T ) ) );
}

///-----------------------------------------------------------------------------
/// @brief Shift elements in range [pSrc, pSrcEnd) into destination starting at pDest.
///        Overlap-safe (memmove-like). Chooses left/right strategy automatically.
/// @return pDest
///-----------------------------------------------------------------------------
template < typename T >
constexpr T *ShiftElements( T *pDest, const T *pSrc, const T *pSrcEnd ) noexcept
{
	BALL_ASSERT_MESSAGE( pSrc <= pSrcEnd, "Invalid source range" );

	if ( pDest < pSrc )
		return ShiftElementsLeft( pDest, pSrc, pSrcEnd );
	else if ( pDest > pSrc )
		return ShiftElementsRight( pDest, pSrc, pSrcEnd );
	else
		return pDest;
}

/// @brief Byte-wise compare of @p nCount elements of T (memcmp semantics).
/// @return <0 if a<b, 0 if equal, >0 if a>b (first differing byte decides).
/// @note    Precondition: if nCount > 0 then @p pLeft and @p pRight must be non-null.
///          Comparison is performed on raw bytes of the objects (not value-wise for T).
template < typename I, typename T >
constexpr int8_t CompareElements( const I nCount, const T *pLeft, const T *pRight )
{
	BALL_ASSERT( I( 0 ) <= nCount );
	BALL_ASSERT_MESSAGE( nCount == I( 0 ) || ( pLeft != nullptr && pRight != nullptr ), "Empty elements" );

	if ( pLeft == pRight || nCount == I( 0 ) )
		return 0;

	const size_t nSize = static_cast< size_t >( nCount ) * sizeof( T );

	const uchar_t *_pLeft = reinterpret_cast< const uchar_t * >( pLeft );
	const uchar_t *_pRight = reinterpret_cast< const uchar_t * >( pRight );

	for ( size_t i = 0; i < nSize; ++i )
	{
		const uchar_t va = _pLeft[ i ];
		const uchar_t vb = _pRight[ i ];

		if ( va != vb )
			return ( va < vb ) ? -1 : 1;
	}

	return 0;
}

#endif // !defined( _INCLUDE_BALL_TYPES_ELEMENTS_HPP_ )
