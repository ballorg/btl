#ifndef _INCLUDE_BALL_TYPES_MEMORYVIEWBASE_HPP_
#	define _INCLUDE_BALL_TYPES_MEMORYVIEWBASE_HPP_

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/assert.h"
#	include "meta/enableif.hpp"
#	include "meta/indexof.hpp"
#	include "meta/issame.hpp"
#	include "meta/number.hpp"
#	include "elements.hpp"
#	include "elementspack.hpp"
#	include "math.hpp"

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
	using Number_t      = MNumber< Index_t >;
	using TypeNumber_t  = MNumber< TypeIndex_t >;
	using Pack_t        = CElementsPack< I, N, TI, Ts ... >;
	using Const_t       = CViewBase< Index_t, 0, TypeIndex_t, const Ts... >;

	static constexpr I FIRST_INDEX = I( 0 );
	static constexpr I FIXED_COUNT = Pack_t::FIXED_COUNT;

	/// @brief Special "not found" value.
	static constexpr I INVALID_INDEX = Number_t::INVALID;
	static constexpr I INVALID_TYPE_INDEX = TypeNumber_t::INVALID;

	// --------- meta helpers ----------
protected:
	template< typename T > using IndexOf_t = MIndexOf< TI, T, Ts... >;

	static constexpr TI NUM_TYPES = sizeof...( Ts );

	template < typename T > static constexpr TI TYPE_INDEX = IndexOf_t< T >::VALUE;
	template < typename T > static constexpr bool TYPE_IN_PACK = ( TYPE_INDEX< T > != INVALID_TYPE_INDEX );

	template < typename T > using Enable_t = EnableIf_t< TYPE_IN_PACK< T >, int >;

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
		: CViewBase( I( CN ), static_cast< Ts * >( arrays )... )
	{}

	constexpr CViewBase( const CViewBase &copyFrom ) noexcept { CopyFrom( copyFrom ); }
	constexpr CViewBase( CViewBase &&moveFrom ) noexcept { MoveFrom( Move( moveFrom ) ); }
	constexpr CViewBase &operator=( const CViewBase &copyFrom ) noexcept { return CopyFrom( copyFrom ); }
	constexpr CViewBase &operator=( CViewBase &&moveFrom ) noexcept { return MoveFrom( Move( moveFrom ) ); }

	// --------- sizes / byte sizes ----------
	static constexpr size_t Stride() noexcept { return ( size_t( 0 ) + ... + sizeof( Ts ) ); }
	constexpr size_t Size() const noexcept { return static_cast< size_t >( m_nCount ) * Stride(); }
	constexpr I Count() const noexcept { return m_nCount; }
	constexpr bool Empty() const noexcept { return Count() == FIRST_INDEX; }
	template < typename T > constexpr bool IsOverflow( I nCount ) const noexcept { return m_Elements.template IsOverflowByType< T >( nCount ); }
	template < typename T > constexpr bool IsOverflow() const noexcept { return IsOverflow< T >( Count() ); }

	/// @brief Raw base pointer for the given type T from the parameter pack.
	template < typename T, Enable_t< T > = 0 > constexpr T *FixedData() noexcept { return m_Elements.template FixedByType< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *FixedData() const noexcept { return m_Elements.template FixedByType< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr T *Data() noexcept { return m_Elements.template DataByType< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *Data() const noexcept { return m_Elements.template DataByType< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr T *Base() noexcept { return IsOverflow< T >() ? Data< T >() : FixedData< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *Base() const noexcept { return IsOverflow< T >() ? Data< T >() : FixedData< T >(); }

	// --------- element access (typed) ----------
	constexpr bool IsValidIndex( I i ) const noexcept { return i != INVALID_INDEX; }

	template < typename T, Enable_t< T > = 0 >
	constexpr T &At( I i )
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( FIRST_INDEX <= i && i < Count() );

		return Base< T >()[ i ];
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr const T &At( I i ) const
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( FIRST_INDEX <= i && i < Count() );

		return Base< T >()[ i ];
	}

	template < typename T, Enable_t< T > = 0 > constexpr T &operator[]( I i ) { return At< T >( i ); }
	template < typename T, Enable_t< T > = 0 > constexpr const T &operator[]( I i ) const { return At< T >( i ); }

	template < typename T, Enable_t< T > = 0 >
	constexpr const T &Front() const
	{
		BALL_ASSERT( Count() > FIRST_INDEX );

		return Base< T >()[ 0 ];
	}

	template < typename T, Enable_t< T > = 0 >
	constexpr const T &Back() const
	{
		BALL_ASSERT( Count() > FIRST_INDEX );

		return Base< T >()[ Count() - 1 ];
	}

	// --------- iterators (typed) ----------
	template < typename T, Enable_t< T > = 0 > constexpr T *begin() { return Base< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr T *end() { return Base< T >() + Count(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *begin() const noexcept { return Base< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *end() const noexcept { return Base< T >() + Count(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *cbegin() noexcept { return Base< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *cend() noexcept { return Base< T >() + Count(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *cbegin() const noexcept { return Base< T >(); }
	template < typename T, Enable_t< T > = 0 > constexpr const T *cend() const noexcept { return Base< T >() + Count(); }

	// --------- typed find helpers (optional, no memcmp/STL) ----------
	template < typename T, Enable_t< T > = 0 >
	constexpr I Find( const T &value, const I iFrom = FIRST_INDEX ) const noexcept
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

		if ( !p || nCount == FIRST_INDEX )
			return INVALID_INDEX;

		for ( I i = nCount; i > FIRST_INDEX; --i )
			if ( p[ i - 1 ] == value )
				return i - 1;

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
		CopyElements( *this );

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
		CopyElements( other );

		return *this;
	}

	/// @brief Steal another view's data by swapping; leaves @p other empty.
	constexpr CViewBase &MoveFrom( CViewBase &&other ) noexcept
	{
		if ( this == &other )
			return *this;

		Math_Swap( m_nCount, other.m_nCount );
		SwapElements( static_cast< CViewBase & >( other ) );

		return *this;
	}

protected: // internal utils
	template < TI K = 0, typename T0, typename ...Rest >
	constexpr void SetElements( T0 *pFirstElement, Rest *...pNextElements ) noexcept
	{
		I nCount = Count();

		if ( m_Elements.template IsOverflowByIndex< K >( nCount ) )
			m_Elements.template DataByIndex< K, T0 >() = pFirstElement;
		else
			BTL::CopyElements( nCount, m_Elements.template FixedByIndex< K, T0 >(), pFirstElement );

		if constexpr ( K < sizeof...( Rest ) )
			SetElements< K + 1 >( pNextElements... );
	}

	template < TI K = 0 >
	constexpr void CopyElements( const CViewBase &other ) noexcept
	{
		m_Elements.template CopyByIndex< K >( Count(), other.m_Elements );

		if constexpr ( K + 1 < NUM_TYPES )
			CopyElements< K + 1 >( other );
	}

	template < TI K = 0 >
	constexpr void SwapElements( CViewBase &other ) noexcept
	{
		m_Elements.template SwapByIndex< K >( Count(), other.m_Elements );

		if constexpr ( K + 1 < NUM_TYPES )
			SwapElements< K + 1 >( other );
	}

	constexpr void Set( I nCount, Ts *...pElements ) noexcept
	{
		m_nCount = nCount;
		SetElements( pElements... );
	}

	constexpr void Swap( CViewBase &other ) noexcept
	{
		Math_Swap( m_nCount, other.m_nCount );
		SwapElements( other );
	}

private:
	I       m_nCount;
	Pack_t  m_Elements;
}; // class CViewBase

#endif // !defined( _INCLUDE_BALL_TYPES_MEMORYVIEWBASE_HPP_ )
