#ifndef _INCLUDE_BALL_TYPES_ELEMENTS_HPP_
#	define _INCLUDE_BALL_TYPES_ELEMENTS_HPP_

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "meta/removereference.hpp"
#	include "xvalue.hpp"

template < typename T >
constexpr T *ConstructElement( T *pMemory )
{
	return reinterpret_cast< T * >( ::new( static_cast< void * >( pMemory ) ) T );
}

template < typename T, typename ...Ts >
constexpr T *ConstructElement( T *pMemory, Ts &&...args )
{
	return reinterpret_cast< T * >( ::new( static_cast< void * >( pMemory ) ) T( Forward< Ts >( args )... ) );
}

template < typename T >
constexpr void DestructElement( T *pMemory )
{
	pMemory->~T();
}

template < typename T, typename I, I N >
constexpr void DestructElement( T ( *pMemory )[ N ] )
{
	for ( I n = 0; n < N; n++ )
	{
		DestructElement( pMemory[ n ] );
	}
}

///-----------------------------------------------------------------------------
/// @brief Copy elements from [pSrc, pSrcEnd) into destination starting at pDest.
///        Overlap-safe, memmove-like (byte-wise for trivially copyable types).
/// @return pDest
///-----------------------------------------------------------------------------
template < typename T, typename I >
inline T *CopyElements( const I nCount, T *pDest, const T *pSrc ) noexcept
{
	for ( I n = 0; n < nCount; ++n )
		pDest[ n ] = pSrc[ n ];

	return pDest;
}


///-----------------------------------------------------------------------------
/// @brief Copy elements right-to-left: copy nLength elements from pSrc to pDest,
///        starting at the end (index nLength-1 down to 0).
///        Useful when ranges may overlap with pDest >= pSrc.
/// @return pDest
///-----------------------------------------------------------------------------
template < typename T, typename I >
inline T *CopyElementsFromEnd( I nCount, T *pDest, const T *pSrc ) noexcept
{
	for ( I n = nCount; n-- > 0; )
		pDest[ n ] = pSrc[ n ];

	return pDest;
}

///-----------------------------------------------------------------------------
/// @brief Shift elements in range [pSrc, pSrcEnd) into destination starting at pDest.
///        Overlap-safe (memmove-like). Copies left-to-right or right-to-left as needed.
/// @return pDest
///-----------------------------------------------------------------------------
template < typename T >
constexpr T *ShiftElements( T *pDest, const T *pSrc, const T *pSrcEnd ) noexcept
{
	using Diff_t = decltype( pSrcEnd - pSrc );

	const Diff_t nCount = pSrcEnd - pSrc;

	if ( pDest < pSrc )
		return CopyElements( nCount, pDest, pSrc );
	else if ( pDest > pSrc )
		return CopyElementsFromEnd( nCount, pDest, pSrc );
	else
		return pDest;
}

template < typename T >
constexpr void ConstructElements( T *pElement, const T *pEnd ) noexcept
{
	while( pElement < pEnd )
	{
		ConstructElement( pElement );
		pElement++;
	}
}

template < typename T >
constexpr void DestructElements( T *pElement, const T *pEnd ) noexcept
{
	while( pElement < pEnd )
	{
		DestructElement( pElement );
		pElement++;
	}
}

/// @brief Byte-wise compare of @p nCount elements of T (memcmp semantics).
/// @return <0 if a<b, 0 if equal, >0 if a>b (first differing byte decides).
/// @note    Precondition: if nCount > 0 then @p pLeft and @p pRight must be non-null.
///          Comparison is performed on raw bytes of the objects (not value-wise for T).
template < typename T, typename I = size_t >
constexpr int8_t CompareElements( const I nCount, const T *pLeft, const T *pRight )
{
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
