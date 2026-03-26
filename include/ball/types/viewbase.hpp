#ifndef _INCLUDE_BALL_TYPES_VIEWBASE_HPP_
#	define _INCLUDE_BALL_TYPES_VIEWBASE_HPP_

#include "meta/select.hpp"
#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/assert.h"
#	include "meta/enableif.hpp"
#	include "meta/indexof.hpp"
#	include "meta/isintegral.hpp"
#	include "meta/issame.hpp"
#	include "elements.hpp"
#	include "elementspack.hpp"
#	include "fixed.hpp"
#	include "math.hpp"
#	include "number.hpp"

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

	static constexpr I FIRST_INDEX = I( 0 );
	static constexpr I FIXED_COUNT = Pack_t::FIXED_COUNT;

	/// @brief Special "not found" value.
	static constexpr I INVALID_INDEX = Fixed_t::INVALID;
	static constexpr I INVALID_TYPE_INDEX = TypeNumber_t::INVALID;

	// --------- meta helpers ----------
	template< typename T > using IndexOf_t = MIndexOf< TI, T, Ts... >;

	static constexpr TI NUM_TYPES = sizeof...( Ts );

	template < typename T > static constexpr TI TYPE_INDEX = IndexOf_t< T >::VALUE;
	template < typename T > static constexpr bool TYPE_IN_PACK = ( TYPE_INDEX< T > != INVALID_TYPE_INDEX );

	template < typename T > using Enable_t = EnableIf_t< TYPE_IN_PACK< T >, int >;
	template < typename T > using PackedTraits_t = MFixedMetadata< RemoveCV_t< T > >;
	template < typename T > using PackedUnsigned_t = PackedTraits_t< T >::Unsigned_t;
	template < typename T > static constexpr bool TYPE_HAS_PACKED_BITS = PackedTraits_t< T >::IS_PACKED;
	template < typename T > static constexpr bits_t PACKED_BITS = PackedTraits_t< T >::BITS;
	template < typename T > static constexpr bool IS_PACKED_STORAGE = TYPE_HAS_PACKED_BITS< T >;

	template< typename T > static constexpr size_t BYTES = static_cast< size_t >( PackedTraits_t< T >::BYTES );

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

	/// @brief Construct from fixed-size C-arrays (all N must match implicitly).
	template < I CN >
	explicit constexpr CViewBase( const Ts ( &...arrays )[ CN ] ) noexcept
		: CViewBase( CN, static_cast< Ts * >( arrays )... )
	{}

	constexpr CViewBase( const CViewBase &copyFrom ) noexcept { CopyFrom( copyFrom ); }
	constexpr CViewBase( CViewBase &&moveFrom ) noexcept { MoveFrom( Move( moveFrom ) ); }
	constexpr CViewBase &operator=( const CViewBase &copyFrom ) noexcept { return CopyFrom( copyFrom ); }
	constexpr CViewBase &operator=( CViewBase &&moveFrom ) noexcept { return MoveFrom( Move( moveFrom ) ); }

	// --------- sizes / byte sizes ----------
	static constexpr size_t Stride() noexcept { return ( size_t( 0 ) + ... + BYTES< Ts > ); }
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
	template < typename T > constexpr bool IsOverflowBy() const noexcept { return m_Elements.template IsOverflowBy< T >( Count() ); }
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
		constexpr PackedRef_t( CViewBase *pOwner, I i ) noexcept : m_pOwner( pOwner ), m_i( i ) {}
		constexpr operator T() const noexcept { return m_pOwner->template PackedGetValueBy< T >( m_i ); }
		constexpr PackedRef_t &operator=( const T &value ) noexcept { m_pOwner->template PackedSetValueBy< T >( m_i, value ); return *this; }
		constexpr PackedRef_t &operator=( const PackedRef_t &other ) noexcept { return operator=( static_cast< T >( other ) ); }

		template < typename V >
		constexpr PackedRef_t &operator+=( const V &value ) noexcept
		{
			m_pOwner->template PackedSetValue< T >
			(
				m_i,
				static_cast< T >( static_cast< T >( *this ) + static_cast< T >( value ) )
			);
			return *this;
		}

	private:
		CViewBase *m_pOwner;
		I m_i;
	};

	template < typename T, Enable_t< T > = 0 >
	constexpr T PackedAt( I i ) const noexcept
	{
		static_assert( IS_PACKED_STORAGE< T >, "PackedAt<T> is only valid for packed storage types" );
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( FIRST_INDEX <= i && i < Count() );

		return PackedGetValueBy< T >( i );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr void PackedSetAt( I i, const T &value ) noexcept
	{
		static_assert( IS_PACKED_STORAGE< T >, "PackedSetAt<T> is only valid for packed storage types" );
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( FIRST_INDEX <= i && i < Count() );

		PackedSetValue< T >( i, value );
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
			return PackedGetValueBy< T >( i );
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
			return PackedGetValueBy< T >( FIRST_INDEX );
		else
			return BaseBy< T >()[ FIRST_INDEX ];
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr decltype( auto ) Back() const
	{
		BALL_ASSERT( Count() > FIRST_INDEX );

		if constexpr ( TYPE_HAS_PACKED_BITS< T > )
			return PackedGetValueBy< T >( Count() - I( 1 ) );
		else
			return BaseBy< T >()[ Count() - I( 1 ) ];
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
			if ( iFrom >= nCount )
				return INVALID_INDEX;

			for ( I i = iFrom; i < nCount; ++i )
			{
				if ( PackedGetValueBy< T >( i ) == value )
					return i;
			}
		}
		else
		{
			const T *pBase = BaseBy< T >();

			if ( !pBase )
				return INVALID_INDEX;

			for ( const T *it = pBase + iFrom, *itEnd = pBase + nCount; it < itEnd; ++it )
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
			iFrom = nCount - I( 1 );

		if constexpr ( TYPE_HAS_PACKED_BITS< T > )
		{
			for ( I i = iFrom; ; --i )
			{
				if ( PackedGetValueBy< T >( i ) == value )
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

			for ( const T *it = pBase + iFrom, *itBegin = pBase; ; --it )
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

		Swap( m_nCount, other.m_nCount );
		SwapElementsFrom( static_cast< CViewBase & >( other ) );

		return *this;
	}

protected: // Packed methods.
	template < typename T, Enable_t< T > = 0 >
	static constexpr size_t PackedBytesForCountBy( I nCount ) noexcept
	{
		const bits_t nBits = static_cast< bits_t >( nCount ) * PACKED_BITS< T >;

		return static_cast< size_t >( ( nBits + bits_t( 7 ) ) / bits_t( 8 ) );
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
		const bits_t iShift = iBit & bits_t( 7 );

		return ( ( pData[ iByte ] >> iShift ) & uchar_t( 1 ) ) != 0;
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
		const bits_t iShift = iBit & bits_t( 7 );
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

		for ( bits_t n = bits_t( 0 ); n < nBits; ++n )
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

		for ( bits_t n = nBits; n > bits_t( 0 ); --n )
		{
			const bits_t iSrc = iFromBit + ( n - bits_t( 1 ) );

			PackedSetDataBitBy< T >( pData, iSrc + nShiftBits, PackedGetDataBitBy< T >( pData, iSrc ) );
		}

		for ( bits_t n = bits_t( 0 ); n < nShiftBits; ++n )
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

		for ( bits_t n = bits_t( 0 ); n < nBits; ++n )
		{
			const bits_t iDest = iFromBit + n;
			PackedSetDataBitBy< T >( pData, iDest, PackedGetDataBitBy< T >( pData, iDest + nShiftBits ) );
		}

		for ( bits_t n = bits_t( 0 ); n < nShiftBits; ++n )
			PackedSetDataBitBy< T >( pData, iFromBit + nBits + n, false );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr PackedUnsigned_t< T > PackedGetRawBy( I i ) const noexcept
	{
		using Packed_t = PackedTraits_t< T >;
		using U = PackedUnsigned_t< T >;

		U nRaw = U( 0 );
		const bits_t iFromBit = PackedBitOffsetBy< T >( i );
		const bits_t nValueBits = Packed_t::BITS;

		const uchar_t *pData = PackedBaseBy< T >();

		BALL_ASSERT( pData != nullptr );

		for ( bits_t n = bits_t( 0 ); n < nValueBits; ++n )
		{
			if ( PackedGetDataBitBy< T >( pData, iFromBit + n ) )
				nRaw = static_cast< U >( nRaw | static_cast< U >( U( 1 ) << n ) );
		}

		return static_cast< U >( nRaw & Packed_t::VALUE_MASK );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr void PackedSetRawBy( I i, PackedUnsigned_t< T > nRaw ) noexcept
	{
		using Packed_t = PackedTraits_t< T >;
		using U = PackedUnsigned_t< T >;

		const bits_t iFromBit = PackedBitOffsetBy< T >( i );
		const bits_t nValueBits = Packed_t::BITS;

		nRaw = static_cast< U >( nRaw & Packed_t::VALUE_MASK );

		uchar_t *pData = PackedBaseBy< T >();

		BALL_ASSERT( pData != nullptr );

		for ( bits_t n = bits_t( 0 ); n < nValueBits; ++n )
		{
			PackedSetDataBitBy< T >( pData, iFromBit + n, ( ( nRaw >> n ) & U( 1 ) ) != U( 0 ) );
		}
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr T PackedGetValueBy( I i ) const noexcept
	{
		using Packed_t = PackedTraits_t< T >;
		using U = PackedUnsigned_t< T >;

		U nRaw = PackedGetRawBy< T >( i );

		if constexpr ( Packed_t::IS_SIGNED )
		{
			if constexpr ( Packed_t::BITS < Packed_t::STORAGE_BITS )
			{
				const U nSignBit = static_cast< U >( U( 1 ) << ( Packed_t::BITS - bits_t( 1 ) ) );

				if ( ( nRaw & nSignBit ) != U( 0 ) )
					nRaw = static_cast< U >( nRaw | static_cast< U >( ~Packed_t::VALUE_MASK ) );
			}
		}

		return static_cast< T >( nRaw );
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr void PackedSetValueBy( I i, const T &value ) noexcept
	{
		using U = PackedUnsigned_t< T >;

		U nRaw = static_cast< U >( value );

		PackedSetRawBy< T >( i, nRaw );
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
					PackedBytesForCountBy< T0 >( nCount ),
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
			CopyElements( nCount, m_Elements.template FixedBy< K, T0 >(), pFirstElement );
		}

		if constexpr ( K < sizeof...( Rest ) )
			SetElements< K + 1 >( pNextElements... );
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
					PackedBytesForCountBy< T0 >( nCount ),
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
				const size_t nBytes = PackedBytesForCountBy< T0 >( nCount );

				for ( size_t n = size_t( 0 ); n < nBytes; ++n )
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

	constexpr void Swap( CViewBase &other ) noexcept
	{
		Swap( m_nCount, other.m_nCount );
		SwapElementsFrom( other );
	}

private:
	I       m_nCount;
	Pack_t  m_Elements;
}; // class CViewBase

#endif // !defined( _INCLUDE_BALL_TYPES_VIEWBASE_HPP_ )
