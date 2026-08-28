#ifndef _INCLUDE_BALL_TYPES_VIEW_HPP_
#	define _INCLUDE_BALL_TYPES_VIEW_HPP_

#	pragma once

#	if !defined( BALL_ENABLE_MODULE )
#		include "base/arch.h"
#		include "base/fixed.h"
#		include "c/assert.h"
#		include "meta/fixed.hpp"
#		include "meta/isconst.hpp"
#		include "math.hpp"
#		include "viewbase.hpp"
#	endif

template < typename I, typename T, I N = 0 >
class CView : public CViewBase< I, N, size8_t, T >
{
public:
	using Base_t = CViewBase< I, N, size8_t, T >;
	using Element_t = T;
	using Index_t = I;
	using View_t = CView< Index_t, Element_t, N >;
	using ConstView_t = CView< Index_t, const Element_t, N >;
	template< I GN > using GrowableView_t = CView< Index_t, Element_t, GN >;
	template< I GN > using ConstGrowableView_t = CView< Index_t, const Element_t, GN >;
	using Fixed_t = MFixed< Index_t >;
	using Packed_Traits_t = typename Base_t::template Packed_Traits_t< Element_t >;

	using Base_t::Base_t;
	using Base_t::operator=;
	using Base_t::FIRST_INDEX;
	static constexpr I FIXED_COUNT = Base_t::FIRST_FIXED_COUNT;

	/// @brief Special "not found" value.
	static constexpr I INVALID_INDEX = Fixed_t::INVALID;
	static constexpr bool IS_PACKED_STORAGE = Base_t::template TYPE_HAS_PACKED_BITS< Element_t >;

	// --------- basic associated types ----------
	using value_type = T;
	using size_type = I;
	using pointer = const T *;
	using const_pointer = const T *;
	using reference = const T &;
	using const_reference = const T &;
	using iterator = const T *;
	using const_iterator  = const T *;

	// --------- ctors ----------
	explicit constexpr CView( I nCount, T *pElements ) noexcept : Base_t( nCount, pElements ) {}
	constexpr CView() noexcept : CView( FIRST_INDEX, nullptr ) {}
	template < I CN > constexpr CView( T ( &elements )[ CN ] ) noexcept : CView( CN, static_cast< T * >( elements ) ) {}
	constexpr CView( T *pBegin, const T *pEnd ) noexcept : CView( FIRST_INDEX, nullptr )
	{
		uintptr_t nBegin = reinterpret_cast< uintptr_t >( pBegin );
		const uintptr_t nEnd = reinterpret_cast< const uintptr_t >( pEnd );

		Base_t::Set( static_cast< I >( ( nEnd - nBegin ) / sizeof( T ) ), pBegin );
	}

	// --------- sizes / count / base ----------
	using Base_t::Size;
	using Base_t::Count;
	constexpr size_t FixedSize() const noexcept { return Base_t::template FixedSizeBy< T >(); }
	constexpr size_t FixedCapacitySize() const noexcept { return Base_t::template FixedCapacitySizeBy< T >(); }
	constexpr bool IsOverflow( I nCount ) const { return Base_t::template IsOverflowBy< T >( nCount ); }
	constexpr bool IsOverflow() const { return IsOverflow( Count() ); }
	constexpr bool Packed_IsOverflow( I nCount ) const { return Base_t::template Packed_IsOverflowBy< T >( nCount ); }
	constexpr bool Packed_IsOverflow() const { return Packed_IsOverflow( Count() ); }

	constexpr T *FixedData() noexcept { return Base_t::template FixedDataBy< T >(); }
	constexpr const T *FixedData() const noexcept { return Base_t::template FixedDataBy< T >(); }
	constexpr uchar_t *Packed_FixedData() noexcept { return Base_t::template Packed_FixedDataBy< T >(); }
	constexpr const uchar_t *Packed_FixedData() const noexcept { return Base_t::template Packed_FixedDataBy< T >(); }
	constexpr T *Data() noexcept { return Base_t::template DataBy< T >(); }
	constexpr const T *Data() const noexcept { return Base_t::template DataBy< T >(); }
	constexpr uchar_t *Packed_Data() noexcept { return Base_t::template Packed_DataBy< T >(); }
	constexpr const uchar_t *Packed_Data() const noexcept { return Base_t::template Packed_DataBy< T >(); }
	constexpr T *Base() noexcept { return Base_t::template BaseBy< T >(); }
	constexpr const T *Base() const noexcept { return Base_t::template BaseBy< T >(); }
	constexpr uchar_t *Packed_Base() noexcept { return Base_t::template Packed_BaseBy< T >(); }
	constexpr const uchar_t *Packed_Base() const noexcept { return Base_t::template Packed_BaseBy< T >(); }

	static constexpr bits_t Packed_Bits() noexcept { return Base_t::template Packed_BitsBy< T >(); }
	static constexpr size_t Packed_SizeBy( I nCount ) noexcept { return Base_t::template Packed_SizeBy< T >( nCount ); }
	constexpr void Packed_ClearRows( I iFrom, I nRows ) noexcept { return Base_t::template Packed_ClearRowsBy< T >( iFrom, nRows ); }
	constexpr void Packed_ShiftRowsLeftBy( I iFrom, I nRows, I nShiftRows ) noexcept { return Base_t::template Packed_ShiftRowsLeftBy< T >( iFrom, nRows, nShiftRows ); }
	constexpr void Packed_ShiftRowsRightBy( I iFrom, I nRows, I nShiftRows ) noexcept { return Base_t::template Packed_ShiftRowsRightBy< T >( iFrom, nRows, nShiftRows ); }
	constexpr T Packed_Get( I i ) const noexcept { return Base_t::template Packed_GetBy< T >( i ); }
	constexpr void Packed_Set( I i, const T &value ) noexcept { return Base_t::template Packed_SetBy< T >( i, value ); }

	// --------- basic access ----------
	using Base_t::Empty;
	constexpr const T *Get() const noexcept { return Base(); }

	class Ref_t
	{
	public:
		constexpr Ref_t( CView *pOwner, I i ) noexcept : m_pOwner( pOwner ), m_index( i ) {}
		constexpr operator T() const noexcept { return m_pOwner->Get( m_index ); }

		template < typename Q = T, EnableIf_t< !IS_CONST< Q >, int > = 0 >
		constexpr Ref_t &operator=( const T &value ) noexcept { m_pOwner->SetTo( m_index, value ); return *this; }

		template < typename Q = T, EnableIf_t< !IS_CONST< Q >, int > = 0 >
		constexpr Ref_t &operator=( const Ref_t &other ) noexcept { return operator=( static_cast< T >( other ) ); }

	private:
		I m_index;
		CView *m_pOwner;
	};

	// --------- iterators ----------
	constexpr iterator begin() { return Base(); }
	constexpr iterator end() { return Base() + Count(); }
	constexpr const_iterator begin() const noexcept { return Base(); }
	constexpr const_iterator end() const noexcept { return Base() + Count(); }
	constexpr const_iterator cbegin() const noexcept { return Base(); }
	constexpr const_iterator cend() const noexcept { return Base() + Count(); }

	/// @brief Mutable view over current elements.
	constexpr ConstView_t Const() const noexcept { return ConstView_t( Count(), Base() ); }

	/// @brief Implicit conversion to read-only view.
	constexpr operator ConstView_t() const { return Const(); }

	constexpr bool IsValidIndex( I i ) const { return i != INVALID_INDEX; }

	constexpr decltype( auto ) Get( I i )
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( 0 <= i && i < Count() );

		if constexpr ( IS_PACKED_STORAGE )
			return Packed_Get( i );
		else
			return Base_t::template Get< T >( i );
	}

	constexpr decltype( auto ) Get( I i ) const
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( 0 <= i && i < Count() );

		if constexpr ( IS_PACKED_STORAGE )
			return Packed_Get( i );
		else
			return Base_t::template Get< T >( i );
	}

	// --------- element access ----------
	constexpr decltype( auto ) operator[]( I i ) { return Get( i ); }
	constexpr decltype( auto ) operator[]( I i ) const { return Get( i ); }

	constexpr decltype( auto ) Front() const
	{
		BALL_ASSERT( 0 < Count() );

		if constexpr ( IS_PACKED_STORAGE )
			return Packed_Get( 0 );
		else
			return Base_t::template Front< T >();
	}

	constexpr decltype( auto ) Back() const
	{
		BALL_ASSERT( 0 < Count() );

		if constexpr ( IS_PACKED_STORAGE )
			return Packed_Get( Count() - 1 );
		else
			return Base_t::template Back< T >();
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
		return ( nCount >= Count() ) ? *this : CView( nCount, Base() + ( Count() - nCount ) );
	}

	constexpr CView DropFront( I nCount ) const noexcept
	{
		return ( nCount >= Count() ) ? CView() : CView( static_cast< I >( Count() - nCount ), Base() + nCount );
	}

	constexpr CView DropBack( I nCount ) const noexcept
	{
		return ( nCount >= Count() ) ? CView() : CView( static_cast< I >( Count() - nCount ), Base() );
	}

	// --------- prefix / suffix checks ----------
	constexpr bool StartsWith( const ConstView_t &vPrefix ) const noexcept
	{
		const I nPrefixCount = vPrefix.Count();

		if ( nPrefixCount > Count() )
			return false;

		if constexpr ( IS_PACKED_STORAGE )
		{
			for ( I i = 0; i < nPrefixCount; ++i )
				if ( !( Packed_Get( i ) == vPrefix.Packed_Get( i ) ) )
					return false;
		}
		else
		{
			const T *pLeft = Base();
			const T *pRight = vPrefix.Base();

			for ( I i = 0; i < nPrefixCount; ++i )
				if ( !( pLeft[ i ] == pRight[ i ] ) )
					return false;
		}

		return true;
	}

	constexpr bool EndsWith( const ConstView_t &vSuffix ) const noexcept
	{
		const I nCount = Count(), nSuffixCount = vSuffix.Count();

		if ( nSuffixCount > nCount )
			return false;

		const I nOffset = static_cast< I >( nCount - nSuffixCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			for ( I i = 0; i < nSuffixCount; ++i )
				if ( !( Packed_Get( nOffset + i ) == vSuffix.Packed_Get( i ) ) )
					return false;
		}
		else
		{
			const T *pLeft = Base() + nOffset;
			const T *pRight = vSuffix.Base();

			for ( I i = 0; i < nSuffixCount; ++i )
				if ( !( pLeft[ i ] == pRight[ i ] ) )
					return false;
		}

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
		const I nCount = Count();
		const I nViewCount = v.Count();
		const I nStart = ( iFrom < FIRST_INDEX ) ? FIRST_INDEX : iFrom;

		// Empty needle: by convention return clamped start position.
		if ( nViewCount == FIRST_INDEX )
			return nStart;

		// Out-of-range or needle longer than the remaining span.
		if ( nStart > nCount || nViewCount > nCount - nStart )
			return INVALID_INDEX;

		const I iLastStart = nCount - nViewCount;

		if constexpr ( IS_PACKED_STORAGE )
		{
			I i = nStart;

			for ( ; i + Base_t::FIND_BATCH_LAST <= iLastStart; i += Base_t::FIND_BATCH_COUNT )
			{
				const I iBatchEnd = static_cast< I >( i + Base_t::FIND_BATCH_COUNT );
				const I iFound = Base_t::FindBatchForward( i, iBatchEnd, [this, &v]( I j ) constexpr noexcept { return MatchFrom( j, v ); } );

				if ( iFound != iBatchEnd )
					return iFound;
			}

			for ( ; i <= iLastStart; ++i )
			{
				if ( MatchFrom( i, v ) )
					return i;
			}
		}
		else
		{
			const T *pBase = Base();
			const T *it = pBase + nStart;
			const T *itLast = pBase + static_cast< I >( iLastStart + 1 );

			for ( ; it + Base_t::FIND_BATCH_COUNT <= itLast; it += Base_t::FIND_BATCH_COUNT )
			{
				const T *itBatchEnd = it + Base_t::FIND_BATCH_COUNT;
				const T *itFound = Base_t::FindBatchForward( it, itBatchEnd, [this, &v]( const T *itCheck ) constexpr noexcept { return MatchFrom( itCheck, v ); } );

				if ( itFound != itBatchEnd )
					return static_cast< I >( itFound - pBase );
			}

			for ( ; it < itLast; ++it )
			{
				if ( MatchFrom( it, v ) )
					return static_cast< I >( it - pBase );
			}
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
			if ( iFrom < FIRST_INDEX || iFrom > nLastStart )
				return INVALID_INDEX;

			iStart = iFrom;
		}

		if constexpr ( IS_PACKED_STORAGE )
		{
			I i = iStart;

			for ( ; i >= Base_t::FIND_BATCH_LAST; )
			{
				const I iBatchEnd = static_cast< I >( i + 1 );
				const I iFound = Base_t::FindBatchReverse( i, iBatchEnd, [ & ]( I j ) constexpr noexcept { return MatchFrom( j, v ); } );

				if ( iFound != iBatchEnd )
					return iFound;

				if ( i < Base_t::FIND_BATCH_COUNT )
					return INVALID_INDEX;

				i -= Base_t::FIND_BATCH_COUNT;
			}

			for ( ; ; --i )
			{
				if ( MatchFrom( i, v ) )
					return i;

				if ( i == FIRST_INDEX )
					break;
			}
		}
		else
		{
			const T *pBase = Base();
			const T *it = pBase + iStart;

			for ( ; pBase + Base_t::FIND_BATCH_LAST <= it; )
			{
				const T *itBatchEnd = it + 1;
				const T *itFound = Base_t::FindBatchReverse( it, itBatchEnd, [this, &v]( const T *itCheck ) constexpr noexcept { return MatchFrom( itCheck, v ); } );

				if ( itFound != itBatchEnd )
					return static_cast< I >( itFound - pBase );

				if ( it < pBase + Base_t::FIND_BATCH_COUNT )
					return INVALID_INDEX;

				it -= Base_t::FIND_BATCH_COUNT;
			}

			for ( ; ; --it )
			{
				if ( MatchFrom( it, v ) )
					return static_cast< I >( it - pBase );

				if ( it == pBase )
					break;
			}
		}

		return INVALID_INDEX;
	}

	// --------- comparisons ----------
	friend constexpr bool operator==( const CView &a, const CView &b ) noexcept
	{
		if ( a.Count() != b.Count() )
			return false;

		for ( I i = FIRST_INDEX; i < a.Count(); ++i )
			if ( !( a.Get( i ) == b.Get( i ) ) )
				return false;

		return true;
	}

	friend constexpr bool operator!=( const CView &a, const CView &b ) noexcept
	{
		return !( a == b );
	}

	friend constexpr bool operator<( const CView &a, const CView &b ) noexcept
	{
		I n = ( a.Count() < b.Count() ) ? a.Count() : b.Count();

			for ( I i = FIRST_INDEX; i < n; ++i )
			{
				const auto &va = a.Get( i );
				const auto &vb = b.Get( i );

			if ( va < vb )
				return true;

			if ( vb < va )
				return false;
		}

		return a.Count() < b.Count();
	}

	friend constexpr bool operator >( const CView &a, const CView &b ) noexcept { return b < a; }
	friend constexpr bool operator<=( const CView &a, const CView &b ) noexcept { return !( b < a ); }
	friend constexpr bool operator>=( const CView &a, const CView &b ) noexcept { return !( a < b ); }

protected:
	constexpr bool MatchFrom( const T *pHay, ConstView_t v ) const noexcept
	{
		const T *pNeedle = v.Base();
		const I nViewCount = v.Count();

		for ( I k = FIRST_INDEX; k < nViewCount; ++k )
		{
			if ( !( pHay[ k ] == pNeedle[ k ] ) )
				return false;
		}

		return true;
	}

	constexpr bool MatchFrom( I i, ConstView_t v ) const noexcept
	{
		const I nViewCount = v.Count();

		if constexpr ( IS_PACKED_STORAGE )
		{
			for ( I k = FIRST_INDEX; k < nViewCount; ++k )
			{
				if ( !( Packed_Get( i + k ) == v.Packed_Get( k ) ) )
					return false;
			}
		}
		else
		{
			return MatchFrom( Base() + i, v );
		}

		return true;
	}

	constexpr void SetTo( I i, const T &value ) noexcept
	{
		if constexpr ( IS_PACKED_STORAGE )
			Base_t::template Packed_Set< T >( i, value );
		else
			Base_t::template Base< T >()[ i ] = value;
	}

	constexpr void Set( I nCount, T *pElements ) noexcept
	{
		Base_t::Set( nCount, pElements );
	}

	using Base_t::SwapSelf;
	using Base_t::CopyFrom;
	using Base_t::MoveFrom;
}; // class CView

template < typename T > using View_t =      CView< size_t, const T >;
template < typename T > using View8_t =     CView< size8_t, const T >;
template < typename T > using View16_t =    CView< size16_t, const T >;
template < typename T > using View32_t =    CView< size32_t, const T >;
template < typename T > using View64_t =    CView< size64_t, const T >;

template < typename T, size_t N > using BufferView_t =      CView< size_t, const T, N >;
template < typename T, size_t N > using BufferView8_t =     CView< size8_t, const T, N >;
template < typename T, size_t N > using BufferView16_t =    CView< size16_t, const T, N >;
template < typename T, size_t N > using BufferView32_t =    CView< size32_t, const T, N >;
template < typename T, size_t N > using BufferView64_t =    CView< size64_t, const T, N >;

#endif // !defined( _INCLUDE_BALL_TYPES_VIEW_HPP_ )
