#ifndef _INCLUDE_BALL_TYPES_MEMORYVIEWBASE_HPP_
#	define _INCLUDE_BALL_TYPES_MEMORYVIEWBASE_HPP_

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/assert.h"
#	include "meta/enableif.hpp"
#	include "meta/issame.hpp"
#	include "meta/number.hpp"
#	include "meta/pack.hpp"
#	include "math.hpp"

///-----------------------------------------------------------------------------
/// @brief View over multiple parallel arrays with different element types,
///        sharing the same element count (SoA-style).
///        Example: CMemoryViewBase<I, float, int, char>
///        holds { float*, int*, char* } with common Count().
///-----------------------------------------------------------------------------
template < typename I, typename TI, typename ...Ts >
class CMemoryViewBase
{
public:
	using Index_t       = I;
	using TypeIndex_t   = TI;
	using Number_t      = MNumber< Index_t >;
	using TypeNumber_t  = MNumber< TypeIndex_t >;
	using ConstView_t   = CMemoryViewBase< Index_t, TypeIndex_t, const Ts... >;

	/// @brief Special "not found" value.
	static constexpr I INVALID_INDEX = Number_t::INVALID;
	static constexpr I INVALID_TYPE_INDEX = TypeNumber_t::INVALID;

	// --------- meta helpers ----------
private:
	static constexpr TI NUM_TYPES = sizeof...( Ts );

	template < typename T, typename U, typename ...Rest >
	struct IndexOf_t
	{
		static constexpr TI VALUE = IS_SAME< T, U > ? 0u : ( 1u + IndexOf_t< T, Rest... >::VALUE );
	};

	template < typename T, typename U >
	struct IndexOf_t< T, U >
	{
		static constexpr TI VALUE = IS_SAME< T, U > ? 0u : INVALID_TYPE_INDEX;
	};

	template < typename T > static constexpr TI TYPE_INDEX = IndexOf_t< T, Ts... >::VALUE;
	template < typename T > static constexpr bool TYPE_IN_PACK = ( TYPE_INDEX< T > != INVALID_TYPE_INDEX );

	template < typename T >
	using Enable_t = EnableIf_t< TYPE_IN_PACK< T >, int >;

	// --------- state ----------
public:
	constexpr CMemoryViewBase() noexcept : m_nCount( I( 0 ) )
	{
		ResetElements< 0 >();
	}

	/// @brief Construct from count and per-type pointers in the same order as Ts...
	explicit constexpr CMemoryViewBase( I nCount, Ts *...pElements ) noexcept
		: m_nCount( nCount )
	{
		SetElements< 0 >( pElements... );
	}

	/// @brief Construct from fixed-size C-arrays (all N must match implicitly).
	template < I N >
	explicit constexpr CMemoryViewBase( const Ts ( &...arrays )[ N ] ) noexcept
		: CMemoryViewBase( I( N ), static_cast< Ts * >( arrays )... )
	{}

	constexpr CMemoryViewBase( const CMemoryViewBase &copyFrom ) noexcept : CMemoryViewBase() { CopyFrom( copyFrom ); }
	constexpr CMemoryViewBase( CMemoryViewBase &&moveFrom ) noexcept { MoveFrom( Move( moveFrom ) ); }
	constexpr CMemoryViewBase &operator=( const CMemoryViewBase &copyFrom ) noexcept { return CopyFrom( copyFrom ); }
	constexpr CMemoryViewBase &operator=( CMemoryViewBase &&moveFrom ) noexcept { return MoveFrom( moveFrom ); }

	// --------- sizes / byte sizes ----------
	static constexpr size_t Stride() noexcept { return ( size_t( 0 ) + ... + sizeof( Ts ) ); }
	constexpr size_t Size() const noexcept { return static_cast< size_t >( m_nCount ) * Stride(); }
	template < typename T, Enable_t< T > = 0 > constexpr size_t SizeOf() const noexcept { return static_cast< size_t >( m_nCount ) * sizeof(T); }
	constexpr I Count() const noexcept { return m_nCount; }
	constexpr bool Empty() const noexcept { return Count() == I( 0 ); }

	/// @brief Raw base pointer for the given type T from the parameter pack.
	template < typename T, Enable_t< T > = 0 > constexpr T *Base() noexcept { return m_Elements.template GetByType< T * >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *Base() const noexcept { return m_Elements.template GetByType< T * >(); }
	template < typename T, Enable_t< T > = 0 > constexpr T *Data() noexcept { return Base< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *Data() const noexcept { return Base< T >(); }

	// --------- element access (typed) ----------
	constexpr bool IsValidIndex( I i ) const noexcept { return i != INVALID_INDEX; }

	template < typename T, Enable_t< T > = 0 >
	constexpr T &At( I i )
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( I( 0 ) <= i && i < Count() );

		return Base< T >()[ i ];
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr const T &At( I i ) const
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( I( 0 ) <= i && i < Count() );

		return Base< T >()[ i ];
	}

	template < typename T, Enable_t< T > = 0 > constexpr T &operator[]( I i ) { return At< T >( i ); }
	template < typename T, Enable_t< T > = 0 > constexpr const T &operator[]( I i ) const { return At< T >( i ); }

	template < typename T, Enable_t< T > = 0 >
	constexpr const T &Front() const
	{
		BALL_ASSERT( Count() > I( 0 ) );

		return Base< T >()[ 0 ];
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr const T &Back() const
	{
		BALL_ASSERT( Count() > I( 0 ) );

		return Base< T >()[ Count() - 1 ];
	}

	// --------- iterators (typed) ----------
	template < typename T, Enable_t< T > = 0 > constexpr T *begin() { return Data< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr T *end() { return Data< T >() + Count(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *begin() const noexcept { return Data< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *end() const noexcept { return Data< T >() + Count(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *cbegin() const noexcept { return Data< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *cend() const noexcept { return Data< T >() + Count(); }

	// --------- typed find helpers (optional, no memcmp/STL) ----------
	template < typename T, Enable_t< T > = 0 >
	constexpr I Find( const T &value, const I iFrom = I( 0 ) ) const noexcept
	{
		const I nCount = Count();
		const T *p = Base< T >();

		if ( !p || iFrom >= nCount )
			return INVALID_INDEX;

		for ( I i = iFrom; i < nCount; ++i )
			if ( p[ i ] == value )
				return i;

		return INVALID_INDEX;
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr I RFind( const T &value ) const noexcept
	{
		const I nCount = Count();
		const T *p = Base< T >();

		if ( !p || nCount == I( 0 ) )
			return INVALID_INDEX;

		for ( I i = nCount; i > I( 0 ); --i )
			if ( p[ i - 1 ] == value )
				return i - 1;

		return INVALID_INDEX;
	}

	// --------- slicing / subviews (typed pointers are advanced equally) ----------
	constexpr CMemoryViewBase Subview( I nPos, I nCount ) const noexcept
	{
		if ( nPos >= m_nCount )
			return CMemoryViewBase();

		const I nMax = static_cast< I >( m_nCount - nPos );
		const I nTake = ( nCount < nMax ) ? nCount : nMax;

		CMemoryViewBase v;

		v.m_nCount = nTake;
		AdvanceAllInto( v, nPos );

		return v;
	}

	constexpr CMemoryViewBase First( I nCount ) const noexcept
	{
		return ( nCount >= m_nCount ) ? *this : Subview( I( 0 ), nCount );
	}

	constexpr CMemoryViewBase Last( I nCount ) const noexcept
	{
		if ( nCount >= m_nCount )
			return *this;
		const I start = static_cast< I >( m_nCount - nCount );
		return Subview( start, nCount );
	}

	constexpr CMemoryViewBase DropFront( I nCount ) const noexcept
	{
		if ( nCount >= m_nCount )
			return CMemoryViewBase();

		return Subview( nCount, static_cast< I >( m_nCount - nCount ) );
	}

	constexpr CMemoryViewBase DropBack( I nCount ) const noexcept
	{
		if ( nCount >= m_nCount )
			return CMemoryViewBase();

		return First( static_cast< I >( m_nCount - nCount ) );
	}

	// --------- views / conversions ----------
	constexpr CMemoryViewBase &View() noexcept { return *this; }
	constexpr const CMemoryViewBase &View() const noexcept { return *this; }
	constexpr ConstView_t ConstView() const noexcept
	{
		ConstView_t v;

		v.m_nCount = m_nCount;
		CopyElementsConstInto( v );

		return v;
	}

	constexpr operator ConstView_t() const { return ConstView(); }

protected:
	// internal utils
	template < TI K >
	constexpr void ResetElements() noexcept
	{
		if constexpr ( K < NUM_TYPES )
		{
			m_Elements.Reset();
			ResetElements< K + 1 >();
		}
	}

	template < TI K, typename T0, typename ...Rest >
	constexpr void SetElements( T0 *pFirstElement, Rest *...pNextElements ) noexcept
	{
		m_Elements.template SetByIndex< K >( pFirstElement );

		if constexpr ( sizeof...( Rest ) > 0 )
			SetElements< K + 1 >( pNextElements... );
	}

	constexpr void CopyElementsFrom( const CMemoryViewBase &other ) noexcept
	{
		m_Elements = other.m_Elements;
	}

	template < typename ...CTs >
	constexpr CMemoryViewBase< I, TI, CTs... > CopyElementsConstInto( CMemoryViewBase< I, TI, CTs... > &out ) const noexcept
	{
		for ( TI i = 0; i < NUM_TYPES; ++i )
			out.m_Elements[ i ] = m_Elements[ i ];
	}

	constexpr void AdvanceAllInto( CMemoryViewBase &out, I nAdvance ) const noexcept
	{
		out.ResetElements< 0 >();

		for ( TI i = 0; i < NUM_TYPES; ++i )
			out.m_Elements[ i ] = AdvanceRaw( m_Elements[ i ], i, nAdvance );
	}

	static constexpr void *AdvanceRaw( void *pData, TI iType, I nAdvance ) noexcept
	{
		// Dispatch by index -> sizeof of the corresponding type in Ts...
		return AdvanceRawImpl< 0, Ts... >( pData, iType, nAdvance );
	}

	template < TI K, typename T0, typename ...Rest >
	static constexpr void *AdvanceRawImpl( void *pData, TI nIndex, I nAdvance ) noexcept
	{
		if ( nIndex == K )
		{
			auto *pTyped = reinterpret_cast< T0 * >( pData );

			return reinterpret_cast< void * >( pTyped ? ( pTyped + nAdvance ) : nullptr );
		}
		if constexpr ( sizeof...( Rest ) > 0 )
			return AdvanceRawImpl< K + 1, Rest... >( pData, nIndex, nAdvance );
		else
			return pData; // unreachable for valid idx
	}

	constexpr void Set( I nCount, Ts *...pElements ) noexcept
	{
		m_nCount = nCount;
		SetElements< 0 >( pElements... );
	}

	constexpr void Swap( CMemoryViewBase &other ) noexcept
	{
		Math_Swap( m_nCount, other.m_nCount );

		for ( TI i = 0; i < NUM_TYPES; ++i )
			Math_Swap( m_Elements[ i ], other.m_Elements[ i ] );
	}

	// --------- copying / moving ----------
	/// @brief Replace this view with another view (shallow copy of pointer + length).
	constexpr CMemoryViewBase &CopyFrom( const CMemoryViewBase &other ) noexcept
	{
		if ( this == &other )
			return *this;

		m_nCount = other.m_nCount;
		CopyElementsFrom( other );

		return *this;
	}

	/// @brief Steal another view's data by swapping; leaves @p other empty.
	constexpr CMemoryViewBase &MoveFrom( CMemoryViewBase &&other ) noexcept
	{
		if ( this == &other )
			return *this;

		Swap( static_cast< CMemoryViewBase & >( other ) );

		return *this;
	}

private:
	I                          m_nCount;
	MPackPointer< TI, Ts ... > m_Elements;
}; // class CMemoryViewBase

#endif // !defined( _INCLUDE_BALL_TYPES_MEMORYVIEWBASE_HPP_ )
