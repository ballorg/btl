#ifndef _INCLUDE_BALL_TYPES_VIEW_HPP_
#	define _INCLUDE_BALL_TYPES_VIEW_HPP_

#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/assert.h"
#	include "meta/fixed.hpp"
#	include "math.hpp"

#	include "viewbase.hpp"

template < typename I, typename T, I N = 0 >
class CView : public CViewBase< I, N, uint8_t, T >
{
public:
	using Base_t        = CViewBase< I, N, uint8_t, T >;
	using Element_t     = T;
	using Index_t       = I;
	using View_t        = CView< Index_t, Element_t, N >;
	using ConstView_t   = CView< Index_t, const Element_t, N >;
	template< I GN > using GrowableView_t = CView< Index_t, Element_t, GN >;
	template< I GN > using ConstGrowableView_t = CView< Index_t, const Element_t, GN >;
	using Fixed_t       = MFixed< Index_t >;

	using Base_t::FIRST_INDEX;
	using Base_t::FIXED_COUNT;

	/// @brief Special "not found" value.
	static constexpr I INVALID_INDEX = Fixed_t::INVALID;

	// --------- basic associated types ----------
	using value_type      = T;
	using size_type       = I;
	using pointer         = const T *;
	using const_pointer   = const T *;
	using reference       = const T &;
	using const_reference = const T &;
	using iterator        = const T *;
	using const_iterator  = const T *;

	// --------- ctors ----------
	explicit constexpr CView( I nCount, T *pElements ) noexcept : Base_t( nCount, pElements ) {}
	constexpr CView() noexcept : CView( FIRST_INDEX, nullptr ) {}
	template < size_t CN > constexpr CView( T ( &elements )[ CN ] ) noexcept : CView( I( CN ), static_cast< T * >( elements ) ) {}
	constexpr CView( T &element ) noexcept : Base_t( { element } ) {}
	constexpr CView( T *pBegin, const T *pEnd ) noexcept : CView( FIRST_INDEX, nullptr )
	{
		uintptr_t nBegin = reinterpret_cast< uintptr_t >( pBegin );
		const uintptr_t nEnd = reinterpret_cast< const uintptr_t >( pEnd );

		Base_t::Set( static_cast< I >( ( nEnd - nBegin ) / sizeof( T ) ), pBegin );
	}

	// Copy / Move
	constexpr CView( const CView &copyFrom ) noexcept { CopyFrom( copyFrom ); }
	constexpr CView( CView &&moveFrom ) noexcept { MoveFrom( Move( moveFrom ) ); }
	constexpr CView &operator=( const CView &copyFrom ) noexcept { return CopyFrom( copyFrom ); }
	constexpr CView &operator=( CView &&moveFrom ) noexcept { return MoveFrom( moveFrom ); }

	// --------- sizes / count / base ----------
	using Base_t::Size;
	using Base_t::Count;
	constexpr const bool IsOverflow( I nCount ) const { return Base_t::template IsOverflow< T >( nCount ); }
	constexpr const bool IsOverflow() const           { return IsOverflow( Count() ); }

	constexpr T        *FixedData() noexcept          { return Base_t::template FixedData< T >(); }
	constexpr const T  *FixedData() const noexcept    { return Base_t::template FixedData< T >(); }
	constexpr T        *Data() noexcept               { return Base_t::template Data< T >(); }
	constexpr const T  *Data() const noexcept         { return Base_t::template Data< T >(); }
	constexpr T        *Base() noexcept               { return Base_t::template Base< T >(); }
	constexpr const T  *Base() const noexcept         { return Base_t::template Base< T >(); }

	// --------- basic access ----------
	using Base_t::Empty;
	constexpr const T *Get() const noexcept       { return Base_t::template Get< T >(); }

	// --------- iterators ----------
	constexpr iterator begin()                        { return Base(); }
	constexpr iterator end()                          { return Base() + Count(); }
	constexpr const_iterator begin() const noexcept   { return Base(); }
	constexpr const_iterator end() const noexcept     { return Base() + Count(); }
	constexpr const_iterator cbegin() const noexcept  { return Base(); }
	constexpr const_iterator cend() const noexcept    { return Base() + Count(); }

	/// @brief Mutable view over current elements.
	constexpr ConstView_t Const() const noexcept { return ConstView_t( Count(), Base() ); }

	/// @brief Implicit conversion to read-only view.
	constexpr operator ConstView_t() const { return Const(); }

	constexpr bool IsValidIndex( I i ) const { return i != INVALID_INDEX; }

	constexpr const T &At( I i ) const
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( 0 <= i && i < Count() );

		return Base()[ i ];
	}

	// --------- element access ----------
	constexpr T &operator[]( I i ) { return At( i ); }
	constexpr const T &operator[]( I i ) const { return At( i ); }

	constexpr const T &Front() const
	{
		BALL_ASSERT( 0 < Count() );

		return Base()[ 0 ];
	}

	constexpr const T &Back() const
	{
		BALL_ASSERT( 0 < Count() );

		return Base()[ Count() - 1 ];
	}

	// --------- slicing / subviews ----------
	constexpr CView Subview( I nPos, I nCount ) const noexcept
	{
		if ( nPos >= Count() )
			return CView();

		I nMaxCount = static_cast< I >( Count() - nPos );
		I nTake = ( nCount < nMaxCount ) ? nCount : nMaxCount;

		return CView( nTake, Data() + nPos );
	}

	constexpr CView First( I nCount ) const noexcept
	{
		return ( nCount >= Count() ) ? *this : CView( nCount, Base() );
	}

	constexpr CView Last( I nCount ) const noexcept
	{
		return ( nCount >= Count() ) ? *this
		                             : CView( nCount, Base() + ( Count() - nCount ) );
	}

	constexpr CView DropFront( I nCount ) const noexcept
	{
		return ( nCount >= Count() ) ? CView()
		                             : CView( static_cast< I >( Count() - nCount ), Base() + nCount );
	}

	constexpr CView DropBack( I nCount ) const noexcept
	{
		return ( nCount >= Count() ) ? CView()
		                             : CView( static_cast< I >( Count() - nCount ), Base() );
	}

	// --------- prefix / suffix checks ----------
	constexpr bool StartsWith( const ConstView_t &vPrefix ) const noexcept
	{
		I nPrefixCount = vPrefix.Count();

		if ( nPrefixCount > Count() )
			return false;

		for ( I i = 0; i < nPrefixCount; ++i )
			if ( !( Base()[ i ] == vPrefix.Base()[ i ] ) )
				return false;

		return true;
	}

	constexpr bool EndsWith( const ConstView_t &vSuffix ) const noexcept
	{
		I nCount = Count(), nSuffixCount = vSuffix.Count();

		if ( nSuffixCount > nCount )
			return false;

		I nOffset = static_cast< I >( nCount - nSuffixCount );

		for ( I i = 0; i < nSuffixCount; ++i )
			if ( !( Base()[ nOffset + i ] == vSuffix.Base()[ i ] ) )
				return false;

		return true;
	}

	constexpr I Find( const T &value, const I iFrom = FIRST_INDEX ) const noexcept { return Base_t::template FindBy< T >( value, iFrom ); }
	constexpr I RFind( const T &value, const I iFrom = INVALID_INDEX ) const noexcept { return Base_t::template RFindBy< T >( value, iFrom ); }

	/// @brief Find first occurrence of subelement @p needle starting at @p from.
	///        Returns INVALID_INDEX if not found.
	///        Element-wise comparison; works for non-trivial T.
	///        No STL / memcmp.
	constexpr I Find( const ConstView_t v, const I iFrom = FIRST_INDEX ) const noexcept
	{
		const I nCount     = Count();
		const I nViewCount = v.Count();
		const T *pData     = Base();
		const T *pViewBase = v.Base();

		// Empty haystack: nothing to find.
		if ( !pData )
			return INVALID_INDEX;

		// Empty needle: by convention return clamped start position.
		if ( nViewCount == FIRST_INDEX )
			return iFrom;

		// Out-of-range or needle longer than the remaining span.
		if ( iFrom > nCount || nViewCount > nCount - iFrom )
			return INVALID_INDEX;

		const T *itStart = pData + iFrom;
		const T *itLastStart = pData + ( nCount - nViewCount );
		const T needleFirst = pViewBase[ 0 ];

		for ( const T *it = itStart; it <= itLastStart; ++it )
		{
			// Quick check on the first element.
			if ( !( *it == needleFirst ) )
				continue;

			// Verify the rest of the needle.
			const T *h = it + I( 1 );
			const T *n = pViewBase + I( 1 );
			const T *nEnd = pViewBase + nViewCount;

			for ( ; n < nEnd; ++n, ++h )
			{
				if ( !( *h == *n ) )
					break;
			}

			if ( n == nEnd )
				return static_cast< I >( it - pData );
		}

		return INVALID_INDEX;
	}

	///-----------------------------------------------------------------------------
	/// @brief Find last (rightmost) occurrence of subrange @p v scanning **backward**.
	///
	/// Search semantics:
	///   - If @p nFrom == INVALID_INDEX, search starts from the last valid start
	///     position (i.e., Count() - v.Count()) and proceeds down to zero.
	///   - Otherwise, search starts at @p nFrom and proceeds down to zero.
	///
	/// Return value:
	///   - Index of the first element of the matched subrange (its start).
	///   - INVALID_INDEX if not found or on invalid inputs.
	///
	/// Notes:
	///   - Element-wise comparison; works with non-trivial T (no memcmp / STL).
	///   - Empty needle convention: returns the clamped start position
	///     (see below) if it is within [0..Count()], otherwise INVALID_INDEX.
	///-----------------------------------------------------------------------------
	constexpr I RFind( const ConstView_t v, const I iFrom = INVALID_INDEX ) const noexcept
	{
		const I nCount     = Count();
		const I nViewCount = v.Count();
		const T *pData     = Base();
		const T *pViewBase = v.Base();

		// Empty haystack: nothing to find.
		if ( !pData )
			return INVALID_INDEX;

		// Handle empty needle: by convention return the clamped start position.
		// Start position for empty needle is:
		//   - last element + 1 when nFrom == INVALID_INDEX  -> nCount
		//   - otherwise the provided nFrom
		if ( nViewCount == FIRST_INDEX )
		{
			const I nStart = ( iFrom == INVALID_INDEX ) ? nCount : iFrom;

			return ( nStart <= nCount ) ? nStart : INVALID_INDEX;
		}

		// Non-empty needle longer than haystack: impossible to match.
		if ( nViewCount > nCount )
			return INVALID_INDEX;

		// The last valid start index where the needle could fit.
		const I nLastStart = static_cast< I >( nCount - nViewCount );

		// Determine the initial backward start index.
		// If nFrom == INVALID_INDEX -> start from nLastStart.
		// Otherwise require nFrom to be within [0..nLastStart].
		I iStart = FIRST_INDEX;

		if ( iFrom == INVALID_INDEX )
		{
			iStart = nLastStart;
		}
		else
		{
			// Out of range start index makes matching impossible.
			if ( iFrom > nLastStart )
				return INVALID_INDEX;

			iStart = iFrom;
		}

		const T *itBegin = pData;
		const T *it = pData + iStart;
		const T needleFirst = pViewBase[ 0 ];

		// Backward scan: iStart .. 0
		for ( ; ; --it )
		{
			// Quick check on the first element at this start.
			if ( *it == needleFirst )
			{
				// Verify the rest of the needle forward from i.
				const T *h = it + I( 1 );
				const T *n = pViewBase + I( 1 );
				const T *nEnd = pViewBase + nViewCount;

				for ( ; n < nEnd; ++n, ++h )
				{
					if ( !( *h == *n ) )
						break;
				}

				if ( n == nEnd )
					return static_cast< I >( it - pData );
			}

			// Stop when i == 0 to avoid unsigned underflow.
			if ( it == itBegin )
				break;
		}

		return INVALID_INDEX;
	}

	// --------- comparisons ----------
	friend constexpr bool operator==( const CView &a, const CView &b ) noexcept
	{
		if ( a.m_nCount != b.m_nCount )
			return false;

		for ( I i = 0; i < a.m_nCount; ++i )
			if ( !( a.m_pElements[ i ] == b.m_pElements[ i ] ) )
				return false;

		return true;
	}

	friend constexpr bool operator!=( const CView &a, const CView &b ) noexcept
	{
		return !( a == b );
	}

	friend constexpr bool operator<( const CView &a, const CView &b ) noexcept
	{
		I n = ( a.m_nCount < b.m_nCount ) ? a.m_nCount : b.m_nCount;

		for ( I i = 0; i < n; ++i )
		{
			const T &va = a.m_pElements[ i ];
			const T &vb = b.m_pElements[ i ];

			if ( va < vb )
				return true;

			if ( vb < va )
				return false;
		}

		return a.m_nCount < b.m_nCount;
	}

	friend constexpr bool operator >( const CView &a, const CView &b ) noexcept { return b < a; }
	friend constexpr bool operator<=( const CView &a, const CView &b ) noexcept { return !( b < a ); }
	friend constexpr bool operator>=( const CView &a, const CView &b ) noexcept { return !( a < b ); }

protected:
	constexpr void Set( I nCount, T *pElements ) noexcept
	{
		Base_t::Set( nCount, pElements );
	}

	using Base_t::Swap;
	using Base_t::CopyFrom;
	using Base_t::MoveFrom;
}; // class CView

template < typename T > using View_t =      CView< size_t, const T >;
template < typename T > using View8_t =     CView< uint8_t, const T >;
template < typename T > using View16_t =    CView< uint16_t, const T >;
template < typename T > using View32_t =    CView< uint32_t, const T >;
template < typename T > using View64_t =    CView< uint64_t, const T >;

template < typename T, size_t N > using BufferView_t =      CView< size_t, const T, N >;
template < typename T, size_t N > using BufferView8_t =     CView< uint8_t, const T, N >;
template < typename T, size_t N > using BufferView16_t =    CView< uint16_t, const T, N >;
template < typename T, size_t N > using BufferView32_t =    CView< uint32_t, const T, N >;
template < typename T, size_t N > using BufferView64_t =    CView< uint64_t, const T, N >;

#endif // !defined( _INCLUDE_BALL_TYPES_VIEW_HPP_ )
