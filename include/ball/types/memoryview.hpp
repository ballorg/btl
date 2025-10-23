#ifndef _INCLUDE_BALL_TYPES_MEMORYVIEW_HPP_
#	define _INCLUDE_BALL_TYPES_MEMORYVIEW_HPP_

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/assert.h"
#	include "meta/number.hpp"
#	include "math.hpp"

#	include "memoryviewbase.hpp"

template < typename I, typename T >
class CMemoryView : public CMemoryViewBase< I, uint8_t, T >
{
public:
	using Base_t        = CMemoryViewBase< I, uint8_t, T >;
	using Element_t     = T;
	using Index_t       = I;
	using Const_t       = CMemoryView< Index_t, const Element_t >;
	using Number_t      = MNumber< Index_t >;

	/// @brief Special "not found" value.
	static constexpr I INVALID_INDEX = Number_t::INVALID;

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
	explicit constexpr CMemoryView( I nCount, T *pElements ) noexcept : Base_t( nCount, pElements ) {}
	constexpr CMemoryView() noexcept : CMemoryView( I( 0 ), nullptr ) {}
	template < size_t N > constexpr CMemoryView( T ( &elements )[ N ] ) noexcept : CMemoryView( I( N ), reinterpret_cast< T * >( elements ) ) {}
	explicit constexpr CMemoryView( T &element ) noexcept : Base_t( { element } ) {}
	constexpr CMemoryView( T *pBegin, const T *pEnd ) noexcept : CMemoryView( I( 0 ), nullptr )
	{
		uintptr_t nBegin = reinterpret_cast< uintptr_t >( pBegin );
		const uintptr_t nEnd = reinterpret_cast< const uintptr_t >( pEnd );

		Base_t::Set( static_cast< I >( ( nEnd - nBegin ) / sizeof( T ) ), pBegin );
	}

	// Copy / Move
	constexpr CMemoryView( const CMemoryView &copyFrom ) noexcept : CMemoryView() { CopyFrom( copyFrom ); }
	constexpr CMemoryView( CMemoryView &&moveFrom ) noexcept { MoveFrom( Move( moveFrom ) ); }
	constexpr CMemoryView &operator=( const CMemoryView &copyFrom ) noexcept { return CopyFrom( copyFrom ); }
	constexpr CMemoryView &operator=( CMemoryView &&moveFrom ) noexcept { return MoveFrom( moveFrom ); }

	// --------- sizes / count / base ----------
	using Base_t::Size;
	using Base_t::Count;
	constexpr T        *Base() noexcept               { return Base_t::template Base< T >(); }
	constexpr const T  *Base() const noexcept         { return Base_t::template Base< T >(); }
	constexpr T        *Data() noexcept               { return Base(); }
	constexpr const T  *Data() const noexcept       { return Base(); }

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
	constexpr CMemoryView &View() noexcept { return *this; }
	constexpr const CMemoryView &View() const noexcept { return *this; }

	/// @brief Mutable view over current elements.
	constexpr Const_t Const() const noexcept { return Const_t( Count(), Base() ); }

	/// @brief Implicit conversion to read-only view.
	constexpr operator Const_t() const { return Const(); }

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
	constexpr CMemoryView Subview( I nPos, I nCount ) const noexcept
	{
		if ( nPos >= Count() )
			return CMemoryView();

		I nMaxCount = static_cast< I >( Count() - nPos );
		I nTake = ( nCount < nMaxCount ) ? nCount : nMaxCount;

		return CMemoryView( nTake, Data() + nPos );
	}

	constexpr CMemoryView First( I nCount ) const noexcept
	{
		return ( nCount >= Count() ) ? *this : CMemoryView( nCount, Data() );
	}

	constexpr CMemoryView Last( I nCount ) const noexcept
	{
		return ( nCount >= Count() ) ? *this
		                             : CMemoryView( nCount, Data() + ( Count() - nCount ) );
	}

	constexpr CMemoryView DropFront( I nCount ) const noexcept
	{
		return ( nCount >= Count() ) ? CMemoryView()
		                             : CMemoryView( static_cast< I >( Count() - nCount ), Data() + nCount );
	}

	constexpr CMemoryView DropBack( I nCount ) const noexcept
	{
		return ( nCount >= Count() ) ? CMemoryView()
		                             : CMemoryView( static_cast< I >( Count() - nCount ), Data() );
	}

	// --------- prefix / suffix checks ----------
	constexpr bool StartsWith( Const_t &vPrefix ) const noexcept
	{
		I nPrefixCount = vPrefix.Count();

		if ( nPrefixCount > Count() )
			return false;

		for ( I i = 0; i < nPrefixCount; ++i )
			if ( !( Data()[ i ] == vPrefix.Data()[ i ] ) )
				return false;

		return true;
	}

	constexpr bool EndsWith( Const_t &vSuffix ) const noexcept
	{
		I nCount = Count(), nSuffixCount = vSuffix.Count();

		if ( nSuffixCount > nCount )
			return false;

		I nOffset = static_cast< I >( nCount - nSuffixCount );

		for ( I i = 0; i < nSuffixCount; ++i )
			if ( !( Data()[ nOffset + i ] == vSuffix.Data()[ i ] ) )
				return false;

		return true;
	}

	constexpr I Find( const T &value, const I iFrom = I( 0 ) ) const noexcept { return Base_t::template Find< T >( value, iFrom ); }
	constexpr I RFind( const T &value, const I iFrom = I( 0 ) ) const noexcept { return Base_t::template RFind< T >( value, iFrom ); }

	/// @brief Find first occurrence of subelement @p needle starting at @p from.
	///        Returns INVALID_INDEX if not found.
	///        Element-wise comparison; works for non-trivial T.
	///        No STL / memcmp.
	constexpr I Find( Const_t v, const I iFrom = INVALID_INDEX ) const noexcept
	{
		const I nCount     = Count();
		const I nViewCount = v.Count();
		const T *pData     = Base();
		const T *pViewBase = v.Base();

		// Empty haystack: nothing to find.
		if ( !pData )
			return INVALID_INDEX;

		// Empty needle: by convention return clamped start position.
		if ( nViewCount == I( 0 ) )
			return iFrom;

		// Out-of-range or needle longer than the remaining span.
		if ( iFrom > nCount || nViewCount > nCount - iFrom )
			return INVALID_INDEX;

		const I nLast = static_cast< I >( nCount - nViewCount );

		for ( I n = iFrom; n <= nLast; ++n )
		{
			// Quick check on the first element.
			if ( !( pData[ n ] == pViewBase[ 0 ] ) )
				continue;

			// Verify the rest of the needle.
			I j = I( 1 );

			for ( ; j < nViewCount; ++j )
			{
				if ( !( pData[ n + j ] == pViewBase[ j ] ) )
					break;
			}

			if ( j == nViewCount )
				return n;
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
	constexpr I RFind( Const_t v, const I iFrom = INVALID_INDEX ) const noexcept
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
		if ( nViewCount == I( 0 ) )
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
		I iStart = I( 0 );

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

		// Backward scan: iStart .. 0
		for ( I n = iStart; ; )
		{
			// Quick check on the first element at this start.
			if ( pData[ n ] == pViewBase[ 0 ] )
			{
				// Verify the rest of the needle forward from i.
				I j = I( 1 );

				for ( ; j < nViewCount; ++j )
				{
					if ( !( pData[ n + j ] == pViewBase[ j ] ) )
						break;
				}

				if ( j == nViewCount )
					return n;
			}

			// Stop when i == 0 to avoid unsigned underflow.
			if ( n == I( 0 ) )
				break;

			--n;
		}

		return INVALID_INDEX;
	}

	// --------- comparisons ----------
	friend constexpr bool operator==( const CMemoryView &a, const CMemoryView &b ) noexcept
	{
		if ( a.m_nCount != b.m_nCount )
			return false;

		for ( I i = 0; i < a.m_nCount; ++i )
			if ( !( a.m_pElements[ i ] == b.m_pElements[ i ] ) )
				return false;

		return true;
	}

	friend constexpr bool operator!=( const CMemoryView &a, const CMemoryView &b ) noexcept
	{
		return !( a == b );
	}

	friend constexpr bool operator<( const CMemoryView &a, const CMemoryView &b ) noexcept
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

	friend constexpr bool operator >( const CMemoryView &a, const CMemoryView &b ) noexcept { return b < a; }
	friend constexpr bool operator<=( const CMemoryView &a, const CMemoryView &b ) noexcept { return !( b < a ); }
	friend constexpr bool operator>=( const CMemoryView &a, const CMemoryView &b ) noexcept { return !( a < b ); }

protected:
	constexpr void Set( I nCount, T *pElements ) noexcept
	{
		Base_t::Set( nCount, pElements );
	}

	using Base_t::Swap;
	using Base_t::CopyFrom;
	using Base_t::MoveFrom;
}; // class CMemoryView

#endif // !defined( _INCLUDE_BALL_TYPES_MEMORYVIEW_HPP_ )
