#ifndef _INCLUDE_BALL_TYPES_VIEWBASE_HPP_
#	define _INCLUDE_BALL_TYPES_VIEWBASE_HPP_

#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/assert/static.h"
#	include "c/assert.h"
#	include "meta/enableif.hpp"
#	include "meta/indexof.hpp"
#	include "meta/indextype.hpp"
#	include "meta/isintegral.hpp"
#	include "meta/issame.hpp"
#	include "meta/select.hpp"
#	include "meta/sequence.hpp"
#	include "meta/xvalue.hpp"
#	include "elements.hpp"
#	include "elementspack.hpp"
#	include "fixed.hpp"
#	include "math.hpp"
#	include "number.hpp"

#	ifndef BALL_FIND_BATCH_COUNT
#		define BALL_FIND_BATCH_COUNT 4
#	endif

BALL_STATIC_ASSERT( 0 < BALL_FIND_BATCH_COUNT, "BALL_FIND_BATCH_COUNT must be greater than zero" );

///-----------------------------------------------------------------------------
/// @brief View over multiple parallel arrays with different element types,
///        sharing the same element count (SoA-style).
///        Example: CViewBase<I, float, int, char>
///        holds { float*, int*, char* } with common Count().
///-----------------------------------------------------------------------------
template < typename I, I N, typename TI, typename ...Ts >
class CViewBase
{
public:
	using Index_t       = I;
	using TypeIndex_t   = TI;
	using Fixed_t       = MFixed< Index_t >;
	using TypeNumber_t  = MFixed< TypeIndex_t >;
	using Pack_t        = CElementsPack< I, N, TI, Ts ... >;
	using Const_t       = CViewBase< Index_t, N, TypeIndex_t, const Ts... >;
	template< I GN = N > using Growable_t = CViewBase< Index_t, GN, TypeIndex_t, Ts ... >;

	static constexpr I FIRST_INDEX = 0;
	static constexpr I FIXED_COUNT = N;
	static constexpr I FIRST_FIXED_COUNT = Pack_t::FIXED_COUNT;

	/// @brief Special "not found" value.
	static constexpr I INVALID_INDEX = I( -1 );

	// --------- meta helpers ----------
	template< typename T > using IndexOf_t = MIndexOf< TI, T, Ts... >;
	template< TI K > using TypeByIndex_t = typename MIndexType< TI, K, Ts... >::Type;

	static constexpr size_t NUM_TYPES = sizeof...( Ts );

	template < typename T > static constexpr int TYPE_INDEX = IndexOf_t< T >::VALUE;
	template < typename T > static constexpr bool TYPE_IN_PACK = ( TYPE_INDEX< T > >= 0 );
	template < typename T > using Enable_t = EnableIf_t< TYPE_IN_PACK< T >, int >;
	template < typename T > using PackedTraits_t = MFixedMetadata< RemoveCV_t< T > >;
	template < typename T > using PackedUnsigned_t = PackedTraits_t< T >::Unsigned_t;
	template < typename T > static constexpr bool TYPE_HAS_PACKED_BITS = PackedTraits_t< T >::IS_PACKED;
	template < typename T > static constexpr bits_t PACKED_BITS = PackedTraits_t< T >::BITS;
	template < typename T > static constexpr bool IS_PACKED_STORAGE = TYPE_HAS_PACKED_BITS< T >;

	template< typename T > static constexpr size_t BYTES = static_cast< size_t >( PackedTraits_t< T >::BYTES );
	static constexpr I FIND_BATCH_COUNT = static_cast< I >( BALL_FIND_BATCH_COUNT );
	static constexpr I FIND_BATCH_LAST = static_cast< I >( BALL_FIND_BATCH_COUNT - 1 );
	using FindBatchSequence_t = typename MMakeSequence< BALL_FIND_BATCH_COUNT >::Type;

	// --------- state ----------
public:
	constexpr CViewBase() noexcept : m_nCount( FIRST_INDEX )
	{
	}

	/// @brief Construct from count and per-type pointers in the same order as Ts...
	explicit constexpr CViewBase( I nCount, Ts *...pElements ) noexcept :
		m_nCount( nCount )
	{
		SetElements( pElements... );
	}

	constexpr CViewBase( const CViewBase &copyFrom ) noexcept { CopyFrom( copyFrom ); }
	constexpr CViewBase( CViewBase &&moveFrom ) noexcept { MoveFrom( Move( moveFrom ) ); }
	constexpr CViewBase &operator=( const CViewBase &copyFrom ) noexcept { return CopyFrom( copyFrom ); }
	constexpr CViewBase &operator=( CViewBase &&moveFrom ) noexcept { return MoveFrom( Move( moveFrom ) ); }

	// --------- sizes / byte sizes ----------
	static constexpr size_t Stride() noexcept { return ( 0 + ... + BYTES< Ts > ); }
	constexpr size_t Size() const noexcept { return static_cast< size_t >( m_nCount ) * Stride(); }
	constexpr I Count() const noexcept { return m_nCount; }
	constexpr I FixedCount() const noexcept { return FIXED_COUNT; }
	template < typename T >  constexpr size_t FixedSizeBy() const noexcept { return FixedCount() * sizeof( T ); }
	constexpr I FixedCapacity() const noexcept { return BitCeil_Const( FixedCount() ); }
	template < typename T > constexpr size_t FixedCapacitySizeBy() const noexcept { return FixedCapacity() * sizeof( T ); }
	constexpr bool Empty() const noexcept { return Count() == FIRST_INDEX; }
	template < typename T > constexpr bool IsOverflowBy( I nCount ) const noexcept
	{
		if constexpr ( TYPE_HAS_PACKED_BITS< T > )
			return m_Elements.template IsPackedOverflowBy< T >( nCount );
		else
			return m_Elements.template IsOverflowBy< T >( nCount );
	}
	template < typename T > constexpr bool IsOverflowBy() const noexcept { return IsOverflowBy< T >( Count() ); }
	template < typename T > constexpr bool IsPackedOverflowBy( I nCount ) const noexcept { return m_Elements.template IsPackedOverflowBy< T >( nCount ); }
	template < typename T > constexpr bool IsPackedOverflowBy() const noexcept { return IsPackedOverflowBy< T >( Count() ); }

	/// @brief Raw base pointer for the given type T from the parameter pack.
	template < typename T, Enable_t< T > = 0 > constexpr T *FixedDataBy() noexcept { return m_Elements.template FixedBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *FixedDataBy() const noexcept { return m_Elements.template FixedBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr T *DataBy() noexcept { return m_Elements.template DataBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *DataBy() const noexcept { return m_Elements.template DataBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr uchar_t *PackedFixedDataBy() noexcept { return m_Elements.template PackedFixedBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const uchar_t *PackedFixedDataBy() const noexcept { return m_Elements.template PackedFixedBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr uchar_t *PackedDataBy() noexcept { return m_Elements.template PackedDataBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const uchar_t *PackedDataBy() const noexcept { return m_Elements.template PackedDataBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr T *BaseBy() noexcept { return IsOverflowBy< T >() ? DataBy< T >() : FixedDataBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *BaseBy() const noexcept { return IsOverflowBy< T >() ? DataBy< T >() : FixedDataBy< T >(); }
	template < TI K > constexpr auto BaseBy() noexcept { using T = TypeByIndex_t< K >; return m_Elements.template BaseBy< K, T >( Count() ); }
	template < TI K > constexpr auto BaseBy() const noexcept { using T = TypeByIndex_t< K >; return m_Elements.template BaseBy< K, T >( Count() ); }
	template < typename T, Enable_t< T > = 0 > constexpr uchar_t *PackedBaseBy( I nCount ) noexcept { return m_Elements.template PackedBaseBy< T >( nCount ); }
	template < typename T, Enable_t< T > = 0 > constexpr const uchar_t *PackedBaseBy( I nCount ) const noexcept { return m_Elements.template PackedBaseBy< T >( nCount ); }
	template < typename T, Enable_t< T > = 0 > constexpr uchar_t *PackedBaseBy() noexcept
	{
		if constexpr ( TYPE_HAS_PACKED_BITS< T > )
			return PackedBaseBy< T >( Count() );
		else
			return reinterpret_cast< uchar_t * >( BaseBy< T >() );
	}
	template < typename T, Enable_t< T > = 0 > constexpr const uchar_t *PackedBaseBy() const noexcept
	{
		if constexpr ( TYPE_HAS_PACKED_BITS< T > )
			return PackedBaseBy< T >( Count() );
		else
			return reinterpret_cast< const uchar_t * >( BaseBy< T >() );
	}

	// --------- element access (typed) ----------
	constexpr bool IsValidIndex( I i ) const noexcept { return i != INVALID_INDEX; }

	template < typename T, Enable_t< T > = 0 >
	class PackedRef_t
	{
	public:
		constexpr PackedRef_t( CViewBase *pOwner, I i ) noexcept : m_i( i ), m_pOwner( pOwner ) {}
		constexpr operator T() const noexcept { return m_pOwner->template PackedGetBy< T >( m_i ); }
		constexpr PackedRef_t &operator=( const T &value ) noexcept { m_pOwner->template PackedSetBy< T >( m_i, value ); return *this; }
		constexpr PackedRef_t &operator=( const PackedRef_t &other ) noexcept { return operator=( static_cast< T >( other ) ); }

		template < typename V >
		constexpr PackedRef_t &operator+=( const V &value ) noexcept
		{
			m_pOwner->template PackedSetBy< T >( m_i, static_cast< T >( static_cast< T >( *this ) + static_cast< T >( value ) ) );

			return *this;
		}

	private:
		I m_i;
		CViewBase *m_pOwner;
	};

	template < typename T, Enable_t< T > = 0 >
	constexpr T PackedAt( I i ) const noexcept
	{
		static_assert( IS_PACKED_STORAGE< T >, "PackedAt<T> is only valid for packed storage types" );
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( FIRST_INDEX <= i && i < Count() );

		return PackedGetBy< T >( i );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr void PackedSetAt( I i, const T &value ) noexcept
	{
		static_assert( IS_PACKED_STORAGE< T >, "PackedSetAt<T> is only valid for packed storage types" );
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( FIRST_INDEX <= i && i < Count() );

		PackedSetBy< T >( i, value );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr decltype( auto ) At( I i )
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( FIRST_INDEX <= i && i < Count() );

		if constexpr ( TYPE_HAS_PACKED_BITS< T > )
			return PackedRef_t< T >( this, i );
		else
			return BaseBy< T >()[ i ];
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr decltype( auto ) At( I i ) const
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( FIRST_INDEX <= i && i < Count() );

		if constexpr ( TYPE_HAS_PACKED_BITS< T > )
			return PackedGetBy< T >( i );
		else
			return BaseBy< T >()[ i ];
	}

	template < typename T, Enable_t< T > = 0 > constexpr decltype( auto ) operator[]( I i ) { return At< T >( i ); }
	template < typename T, Enable_t< T > = 0 > constexpr decltype( auto ) operator[]( I i ) const { return At< T >( i ); }

	template < typename T, Enable_t< T > = 0 >
	constexpr decltype( auto ) Front() const
	{
		BALL_ASSERT( Count() > FIRST_INDEX );

		if constexpr ( TYPE_HAS_PACKED_BITS< T > )
			return PackedGetBy< T >( FIRST_INDEX );
		else
			return BaseBy< T >()[ FIRST_INDEX ];
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr decltype( auto ) Back() const
	{
		BALL_ASSERT( Count() > FIRST_INDEX );

		if constexpr ( TYPE_HAS_PACKED_BITS< T > )
			return PackedGetBy< T >( Count() - 1 );
		else
			return BaseBy< T >()[ Count() - 1 ];
	}

	// --------- iterators (typed) ----------
	template < typename T, Enable_t< T > = 0 > constexpr T *begin() { return BaseBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr T *end() { return BaseBy< T >() + Count(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *begin() const noexcept { return BaseBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *end() const noexcept { return BaseBy< T >() + Count(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *cbegin() noexcept { return BaseBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *cend() noexcept { return BaseBy< T >() + Count(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *cbegin() const noexcept { return BaseBy< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *cend() const noexcept { return BaseBy< T >() + Count(); }

	///-----------------------------------------------------------------------------
	/// @brief Find first occurrence of @p value in a single typed column T.
	/// @return Row index or INVALID_INDEX.
	///-----------------------------------------------------------------------------
	template < typename T, Enable_t< T > = 0 >
	constexpr I FindBy( const T &value, const I iFrom = FIRST_INDEX ) const noexcept
	{
		const I nCount = Count();

		if( !nCount || iFrom >= nCount )
			return INVALID_INDEX;

		if constexpr ( TYPE_HAS_PACKED_BITS< T > )
		{
			I i = iFrom;

			for ( ; i + FIND_BATCH_LAST < nCount; i += FIND_BATCH_COUNT )
			{
				const I iBatchEnd = static_cast< I >( i + FIND_BATCH_COUNT );
				const I iFound = FindBatchForward( i, iBatchEnd, [this, &value]( I j ) constexpr noexcept { return PackedGetBy< T >( j ) == value; } );

				if ( iFound != iBatchEnd )
					return iFound;
			}

			for ( ; i < nCount; ++i )
			{
				if ( PackedGetBy< T >( i ) == value )
					return i;
			}
		}
		else
		{
			const T *pBase = BaseBy< T >();
			const T *itEnd = pBase + nCount;

			if ( !pBase )
				return INVALID_INDEX;

			const T *it = pBase + iFrom;

			for ( ; it + FIND_BATCH_COUNT <= itEnd; it += FIND_BATCH_COUNT )
			{
				const T *itBatchEnd = it + FIND_BATCH_COUNT;
				const T *itFound = FindBatchForward( it, itBatchEnd, [&value]( const T *itCheck ) constexpr noexcept { return *itCheck == value; } );

				if ( itFound != itBatchEnd )
					return static_cast< I >( itFound - pBase );
			}

			for ( ; it < itEnd; ++it )
			{
				if ( *it == value )
					return static_cast< I >( it - pBase );
			}
		}

		return INVALID_INDEX;
	}

	///-----------------------------------------------------------------------------
	/// @brief Find last occurrence of @p value in column T searching backward from @p iFrom.
	/// @return Row index or INVALID_INDEX.
	///-----------------------------------------------------------------------------
	template < typename T, Enable_t< T > = 0 >
	constexpr I RFindBy( const T &value, I iFrom = INVALID_INDEX ) const noexcept
	{
		const I nCount = Count();

		if ( nCount == FIRST_INDEX )
			return INVALID_INDEX;

		if ( iFrom == INVALID_INDEX || iFrom >= nCount )
			iFrom = nCount - 1;

		if constexpr ( TYPE_HAS_PACKED_BITS< T > )
		{
			I i = iFrom;

			for ( ; i >= FIND_BATCH_LAST; )
			{
				const I iBatchEnd = static_cast< I >( i + 1 );
				const I iFound = FindBatchReverse( i, iBatchEnd, [&]( I j ) constexpr noexcept { return PackedGetBy< T >( j ) == value; } );

				if ( iFound != iBatchEnd )
					return iFound;

				if ( i < FIND_BATCH_COUNT )
					return INVALID_INDEX;

				i -= FIND_BATCH_COUNT;
			}

			for ( ; ; --i )
			{
				if ( PackedGetBy< T >( i ) == value )
					return i;

				if ( i == FIRST_INDEX )
					break;
			}

			return INVALID_INDEX;
		}
		else
		{
			const T *pBase = BaseBy< T >();

			if ( !pBase )
				return INVALID_INDEX;

			const T *it = pBase + iFrom;
			const T *itBegin = pBase;

			for ( ; itBegin + FIND_BATCH_LAST <= it; )
			{
				const T *itBatchEnd = it + 1;
				const T *itFound = FindBatchReverse( it, itBatchEnd, [&]( const T *itCheck ) constexpr noexcept { return *itCheck == value; } );

				if ( itFound != itBatchEnd )
					return static_cast< I >( itFound - pBase );

				if ( it < itBegin + FIND_BATCH_COUNT )
					return INVALID_INDEX;

				it -= FIND_BATCH_COUNT;
			}

			for ( ; ; --it )
			{
				if ( *it == value )
					return static_cast< I >( it - pBase );

				if ( it == itBegin )
					break;
			}
		}

		return INVALID_INDEX;
	}

	// --------- slicing / subviews (typed pointers are advanced equally) ----------
	constexpr CViewBase Subview( I nPos, I nCount ) const noexcept
	{
		if ( nPos >= m_nCount )
			return CViewBase();

		const I nMax = static_cast< I >( m_nCount - nPos );
		const I nTake = ( nCount < nMax ) ? nCount : nMax;

		CViewBase v;

		v.m_nCount = nTake;
		AdvanceAllInto( v, nPos );

		return v;
	}

	constexpr CViewBase First( I nCount ) const noexcept
	{
		return ( nCount >= m_nCount ) ? *this : Subview( FIRST_INDEX, nCount );
	}

	constexpr CViewBase Last( I nCount ) const noexcept
	{
		if ( nCount >= m_nCount )
			return *this;
		const I start = static_cast< I >( m_nCount - nCount );
		return Subview( start, nCount );
	}

	constexpr CViewBase DropFront( I nCount ) const noexcept
	{
		if ( nCount >= m_nCount )
			return CViewBase();

		return Subview( nCount, static_cast< I >( m_nCount - nCount ) );
	}

	constexpr CViewBase DropBack( I nCount ) const noexcept
	{
		if ( nCount >= m_nCount )
			return CViewBase();

		return First( static_cast< I >( m_nCount - nCount ) );
	}

	// --------- views / conversions ----------
	constexpr Const_t Const() const noexcept
	{
		Const_t v;

		v.m_nCount = m_nCount;
		v.CopyElementsFrom( *this );

		return v;
	}

	constexpr operator Const_t() const { return Const(); }

protected:
	template < typename Iter >
	static constexpr Iter BatchOffset( Iter itBase, int nOffset ) noexcept
	{
		if constexpr ( IS_INTEGRAL< Iter > )
		{
			return nOffset < 0 ? static_cast< Iter >( itBase - static_cast< Iter >( -nOffset ) ) : static_cast< Iter >( itBase + static_cast< Iter >( nOffset ) );
		}
		else
		{
			return nOffset < 0 ? itBase - static_cast< int >( -nOffset ) : itBase + static_cast< int >( nOffset );
		}
	}

	template < typename Iter, typename Pred, size_t ...Is >
	static constexpr Iter FindBatchForwardBy( Iter itBase, Iter itEnd, Pred &&funcCheck, MSequence< Is... > ) noexcept
	{
		Iter itFound = itEnd;

		( ( itFound == itEnd && funcCheck( BatchOffset( itBase, static_cast< int >( Is ) ) ) ? ( itFound = BatchOffset( itBase, static_cast< int >( Is ) ), 0 ) : 0 ), ...);

		return itFound;
	}

	template < typename Iter, typename Pred >
	static constexpr Iter FindBatchForward( Iter itBase, Iter itEnd, Pred &&funcCheck ) noexcept
	{
		return FindBatchForwardBy( itBase, itEnd, Forward< Pred >( funcCheck ), FindBatchSequence_t() );
	}

	template < typename Iter, typename Pred, size_t ...Is >
	static constexpr Iter FindBatchReverseBy( Iter itBase, Iter itEnd, Pred &&funcCheck, MSequence< Is... > ) noexcept
	{
		Iter itFound = itEnd;

		( ( itFound == itEnd && funcCheck( BatchOffset( itBase, -static_cast< int >( Is ) ) ) ? ( itFound = BatchOffset( itBase, -static_cast< int >( Is ) ), 0 ) : 0 ), ... );

		return itFound;
	}

	template < typename Iter, typename Pred >
	static constexpr Iter FindBatchReverse( Iter itBase, Iter itEnd, Pred &&funcCheck ) noexcept
	{
		return FindBatchReverseBy( itBase, itEnd, Forward< Pred >( funcCheck ), FindBatchSequence_t() );
	}

	// --------- copying / moving ----------
	/// @brief Replace this view with another view (shallow copy of pointer + length).
	constexpr CViewBase &CopyFrom( const CViewBase &other ) noexcept
	{
		if ( this == &other )
			return *this;

		m_nCount = other.m_nCount;
		CopyElementsFrom( other );

		return *this;
	}

	template< I GN > constexpr CViewBase &CopyFrom( const Growable_t< GN > &other ) noexcept
	{
		m_nCount = other.Count();
		CopyElementsFrom< GN >( other );

		return *this;
	}

	/// @brief Steal another view's data by swapping; leaves @p other empty.
	constexpr CViewBase &MoveFrom( CViewBase &&other ) noexcept
	{
		if ( this == &other )
			return *this;

		return SwapSelf( other );
	}

protected: // Packed methods.
	template < typename T, Enable_t< T > = 0 >
	static constexpr size_t PackedSizeBy( I nCount ) noexcept
	{
		const bits_t nBits = static_cast< bits_t >( nCount ) * PACKED_BITS< T >;

		return static_cast< size_t >( ( nBits + 7 ) / 8 );
	}

	template < typename T, Enable_t< T > = 0 >
	static constexpr bits_t PackedBitOffsetBy( I i ) noexcept
	{
		return static_cast< bits_t >( i ) * PACKED_BITS< T >;
	}

	template < typename T, Enable_t< T > = 0 >
	static constexpr bool PackedGetDataBitBy( const uchar_t *pData, bits_t iBit ) noexcept
	{
		BALL_ASSERT( pData != nullptr );

		const bits_t iByte = iBit >> 3;
		const bits_t iShift = iBit & 7;

		return ( ( pData[ iByte ] >> iShift ) & 1 ) != 0;
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr bool PackedGetBitBy( bits_t iBit ) const noexcept
	{
		return PackedGetDataBitBy( PackedBaseBy(), iBit );
	}

	template < typename T, Enable_t< T > = 0 >
	static constexpr void PackedSetDataBitBy( uchar_t *pData, bits_t iBit, bool bValue ) noexcept
	{
		BALL_ASSERT( pData != nullptr );

		const bits_t iByte = iBit >> 3;
		const bits_t iShift = iBit & 7;
		const uchar_t nMask = static_cast< uchar_t >( uchar_t( 1 ) << iShift );

		if ( bValue )
			pData[ iByte ] = static_cast< uchar_t >( pData[ iByte ] | nMask );
		else
			pData[ iByte ] = static_cast< uchar_t >( pData[ iByte ] & static_cast< uchar_t >( ~nMask ) );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr void PackedSetBitBy( bits_t iBit, bool bValue ) noexcept
	{
		return PackedSetDataBitBy< T >( PackedBaseBy(), iBit, bValue );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr void PackedClearRowsBy( I iFrom, I nRows ) noexcept
	{
		if ( nRows <= FIRST_INDEX )
			return;

		uchar_t *pData = PackedBaseBy< T >();

		BALL_ASSERT( pData != nullptr );

		const bits_t nBits = static_cast< bits_t >( nRows ) * PACKED_BITS< T >;
		const bits_t iFromBit = PackedBitOffsetBy< T >( iFrom );

		for ( bits_t n = 0; n < nBits; ++n )
			PackedSetDataBitBy< T >( pData, iFromBit + n, false );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr void PackedShiftRowsRightBy( I iFrom, I nRows, I nShiftRows ) noexcept
	{
		if ( nRows <= FIRST_INDEX || nShiftRows <= FIRST_INDEX )
			return;

		const bits_t nValueBits = PACKED_BITS< T >;
		const bits_t nBits = static_cast< bits_t >( nRows ) * nValueBits;
		const bits_t nShiftBits = static_cast< bits_t >( nShiftRows ) * nValueBits;
		const bits_t iFromBit = PackedBitOffsetBy< T >( iFrom );

		uchar_t *pData = PackedBaseBy< T >();

		BALL_ASSERT( pData != nullptr );

		for ( bits_t n = nBits; n > 0; --n )
		{
			const bits_t iSrc = iFromBit + ( n - 1 );

			PackedSetDataBitBy< T >( pData, iSrc + nShiftBits, PackedGetDataBitBy< T >( pData, iSrc ) );
		}

		for ( bits_t n = 0; n < nShiftBits; ++n )
			PackedSetDataBitBy< T >( pData, iFromBit + n, false );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr void PackedShiftRowsLeftBy( I iFrom, I nRows, I nShiftRows ) noexcept
	{
		if ( nRows <= FIRST_INDEX || nShiftRows <= FIRST_INDEX )
			return;

		const bits_t nValueBits = PACKED_BITS< T >;
		const bits_t nBits = static_cast< bits_t >( nRows ) * nValueBits;
		const bits_t nShiftBits = static_cast< bits_t >( nShiftRows ) * nValueBits;
		const bits_t iFromBit = PackedBitOffsetBy< T >( iFrom );

		uchar_t *pData = PackedBaseBy< T >();

		BALL_ASSERT( pData != nullptr );

		for ( bits_t n = 0; n < nBits; ++n )
		{
			const bits_t iDest = iFromBit + n;
			PackedSetDataBitBy< T >( pData, iDest, PackedGetDataBitBy< T >( pData, iDest + nShiftBits ) );
		}

		for ( bits_t n = 0; n < nShiftBits; ++n )
			PackedSetDataBitBy< T >( pData, iFromBit + nBits + n, false );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr T PackedGetBy( I i ) const noexcept
	{
		using Packed_t = PackedTraits_t< T >;
		using U = PackedUnsigned_t< T >;

		constexpr bits_t nBits = Packed_t::BITS;
		const bits_t iFromBit = PackedBitOffsetBy< T >( i );
		const uchar_t *pData = PackedBaseBy< T >();

		BALL_ASSERT( pData != nullptr );

		const uchar_t *pByte = pData + ( iFromBit >> 3 );
		bits_t iShift = iFromBit & 7;

		U nRaw = 0;

		for ( bits_t n = 0; n < nBits; ++n )
		{
			nRaw |= static_cast< U >( static_cast< U >( ( *pByte >> iShift ) & 1 ) << n );

			++iShift;

			if ( iShift == 8 )
			{
				iShift = 0;
				++pByte;
			}
		}

		return static_cast< T >( nRaw & Packed_t::VALUE_MASK );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr void PackedSetBy( I i, const T &value ) noexcept
	{
		using Packed_t = PackedTraits_t< T >;
		using U = PackedUnsigned_t< T >;

		constexpr bits_t nBits = Packed_t::BITS;
		const bits_t iFromBit = PackedBitOffsetBy< T >( i );

		uchar_t *pData = PackedBaseBy< T >();

		BALL_ASSERT( pData != nullptr );

		uchar_t *pByte = pData + ( iFromBit >> 3 );
		bits_t iShift = iFromBit & 7;

		const U nRaw = static_cast< U >( value ) & Packed_t::VALUE_MASK;

		for ( bits_t n = 0; n < nBits; ++n )
		{
			const uchar_t nMask = static_cast< uchar_t >( uchar_t( 1 ) << iShift );

			if ( ( nRaw >> n ) & 1 )
				*pByte = static_cast< uchar_t >( *pByte | nMask );
			else
				*pByte = static_cast< uchar_t >( *pByte & static_cast< uchar_t >( ~nMask ) );

			++iShift;

			if ( iShift == 8 )
			{
				iShift = 0;
				++pByte;
			}
		}
	}

	template < TI K = 0, typename T0, typename ...Rest >
	constexpr void SetElements( T0 *pFirstElement, Rest *...pNextElements ) noexcept
	{
		I nCount = Count();

		if constexpr ( TYPE_HAS_PACKED_BITS< T0 > )
		{
			if ( m_Elements.template IsPackedOverflowBy< K >( nCount ) )
				m_Elements.template PackedDataBy< K, T0 >() = reinterpret_cast< uchar_t * >( const_cast< RemoveCV_t< T0 > * >( pFirstElement ) );
			else
				CopyElements
				(
					PackedSizeBy< T0 >( nCount ),
					m_Elements.template PackedFixedBy< K, T0 >(),
					reinterpret_cast< const uchar_t * >( pFirstElement )
				);
		}
		else if ( m_Elements.template IsOverflowBy< K >( nCount ) )
		{
			m_Elements.template DataBy< K, T0 >() = pFirstElement;
		}
		else
		{
			m_Elements.template StoreFixedElements< K, T0 >( nCount, pFirstElement );
		}

		if constexpr ( 0 < sizeof...( Rest ) )
			SetElements< K + 1 >( pNextElements... );
	}

	///-----------------------------------------------------------------------------
	/// @brief One-time, in-place construction of row 0 directly in the inline buffer.
	/// @details Sets the count to one and writes one value per column into its fixed
	/// buffer, activating the proper union member first so the whole operation is
	/// valid inside a constant expression. The caller guarantees the single row fits
	/// inline (no overflow), which holds for fixed-capacity buffers.
	///-----------------------------------------------------------------------------
	template < TI K = 0, typename T0, typename ...Rest >
	constexpr void StoreFirstElement( const T0 &first, const Rest &...rest ) noexcept
	{
		if constexpr ( K == 0 )
			m_nCount = 1;

		if constexpr ( TYPE_HAS_PACKED_BITS< T0 > )
		{
			m_Elements.template ActivatePackedFixed< K >();
			PackedSetBy< T0 >( 0, first );
		}
		else
		{
			m_Elements.template StoreFixedElements< K, T0 >( 1, &first );
		}

		if constexpr ( 0 < sizeof...( Rest ) )
			StoreFirstElement< K + 1 >( rest... );
	}

	template < TI K = 0, typename T0, typename ...Rest >
	constexpr void CopyElementsBy( const CViewBase &other ) noexcept
	{
		I nCount = Count();

		if constexpr ( TYPE_HAS_PACKED_BITS< T0 > )
		{
			if ( m_Elements.template IsPackedOverflowBy< K >( nCount ) )
				m_Elements.template PackedDataBy< K, T0 >() = other.m_Elements.template PackedDataBy< K, T0 >();
			else
				CopyElements
				(
					PackedSizeBy< T0 >( nCount ),
					m_Elements.template PackedFixedBy< K, T0 >(),
					other.m_Elements.template PackedFixedBy< K, T0 >()
				);
		}
		else
		{
			m_Elements.template CopyBy< K >( nCount, other.m_Elements );
		}

		if constexpr ( 0 < sizeof...( Rest ) )
			CopyElementsBy< K + 1, Rest... >( other );
	}

	constexpr void CopyElementsFrom( const CViewBase &other ) noexcept
	{
		if constexpr ( 0 < NUM_TYPES )
			CopyElementsBy< 0, Ts... >( other );
	}

	template < I GN, TI K = 0 >
	constexpr void CopyElementsFrom( const Growable_t< GN > &other ) noexcept
	{
		m_Elements.template CopyBy< K >( other.Count(), other.m_Elements );

		if constexpr ( K + 1 < NUM_TYPES )
			CopyElementsFrom< K + 1 >( other );
	}

	template < TI K = 0, typename T0, typename ...Rest >
	constexpr void SwapElementsBy( CViewBase &other ) noexcept
	{
		I nCount = Count();

		if constexpr ( TYPE_HAS_PACKED_BITS< T0 > )
		{
			if ( m_Elements.template IsPackedOverflowBy< K >( nCount ) )
				Swap( m_Elements.template PackedDataBy< K, T0 >(), other.m_Elements.template PackedDataBy< K, T0 >() );
			else
			{
				uchar_t *pLeft = m_Elements.template PackedFixedBy< K, T0 >();
				uchar_t *pRight = other.m_Elements.template PackedFixedBy< K, T0 >();
				const size_t nBytes = PackedSizeBy< T0 >( nCount );

				for ( size_t n = 0; n < nBytes; ++n )
					Swap( pLeft[ n ], pRight[ n ] );
			}
		}
		else
		{
			m_Elements.template SwapBy< K >( nCount, other.m_Elements );
		}

		if constexpr ( 0 < sizeof...( Rest ) )
			SwapElementsBy< K + 1, Rest... >( other );
	}

	constexpr void SwapElementsFrom( CViewBase &other ) noexcept
	{
		if constexpr ( 0 < NUM_TYPES )
			SwapElementsBy< 0, Ts... >( other );
	}

	constexpr void Set( I nCount, Ts *...pElements ) noexcept
	{
		m_nCount = nCount;

		if constexpr ( 0 < NUM_TYPES )
			SetElements( pElements... );
	}

	constexpr CViewBase &SwapSelf( CViewBase &other ) noexcept
	{
		Swap( m_nCount, other.m_nCount );
		SwapElementsFrom( other );

		return *this;
	}

private:
	I       m_nCount;
	Pack_t  m_Elements;
}; // class CViewBase

#endif // !defined( _INCLUDE_BALL_TYPES_VIEWBASE_HPP_ )
