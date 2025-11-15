#ifndef _INCLUDE_BALL_TYPES_ELEMENTSPACK_HPP_
#	define _INCLUDE_BALL_TYPES_ELEMENTSPACK_HPP_

#include "elements.hpp"
#	include "base/arch.h"
#	include "base/fixed.h"
#	include "meta/pack.hpp"
#	include "meta/removecv.hpp"
#	include "meta/select.hpp"
#	include "array.hpp"

template < typename I, typename T >
struct MElementsPack
{
	using Type = T;

	static constexpr size_t FIXED_ELEMENT_SIZE = sizeof( Type );
	static constexpr size_t POINTER_SIZE = sizeof( Type * );
	static constexpr bool   FIXED_OVERFLOWS_POINTER = POINTER_SIZE >= FIXED_ELEMENT_SIZE;
	static constexpr I      FIXED_COUNT = I( POINTER_SIZE / FIXED_ELEMENT_SIZE );
};

template < typename I, I N, typename TI, typename ...Ts >
class CElementsPack;

template < typename I, I N, typename TI, typename T0 >
class CElementsPack< I, N, TI, T0 >
{
public:
	using Type = T0;
	using Meta_t = MElementsPack< I, T0 >;

	static constexpr I FIXED_COUNT = N > 0 ? N : Meta_t::FIXED_COUNT;

	using Element_t = RemoveCV_t< Type >;
	using Fixed_t = MSelect< FIXED_COUNT == 0 >::template Apply_t
		<
			CEmptyArray< I, Element_t >,
			CArray< I, Element_t, FIXED_COUNT >
		>
	;
	using Data_t = Type *;

	constexpr CElementsPack() noexcept : m_Node() {}

	static constexpr bool IsOverflow( I nCount ) noexcept { return nCount > FIXED_COUNT; }
	template < typename T > constexpr bool IsOverflowByType( I nCount ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsOverflow( nCount );
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K > constexpr bool IsOverflowByIndex( I nCount ) const noexcept
	{
		if constexpr ( K == 0 )
			return IsOverflow( nCount );
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

	// ---- static storage access (SoA arrays), returns T* ----
	template < typename T > RemoveCV_t< T > *FixedByType() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_Fixed.Data();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > const T *FixedByType() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_Fixed.Data();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > RemoveCV_t< T > *FixedByIndex() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_Fixed.Data();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > const T *FixedByIndex() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_Fixed.Data();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

	// ---- pointer storage access (AoS of pointers), returns T* ----
	template < typename T > Data_t &DataByType() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pData;
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > const Data_t &DataByType() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pData;
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > Data_t &DataByIndex() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pData;
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > const Data_t &DataByIndex() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pData;
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

	template < typename T > T *ByType( I nCount ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsOverflow( nCount ) ? DataByType< T >() : FixedByType< T >();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > const T *ByType( I nCount ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsOverflow( nCount ) ? DataByType< T >() : FixedByType< T >();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}

	template < TI K, typename T > T *ByIndex( I nCount ) noexcept
	{
		if constexpr ( K == 0 )
			return IsOverflow( nCount ) ? DataByIndex< K, T >() : FixedByIndex< K, T >();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > const T *ByIndex( I nCount ) const noexcept
	{
		if constexpr ( K == 0 )
			return IsOverflow( nCount ) ? DataByIndex< K, T >() : FixedByIndex< K, T >();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

	template < typename T > constexpr void CopyByType( I nCount, const CElementsPack &other ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			if ( IsOverflow( nCount ) )
				m_Node.m_pData = other.DataByType< T >();
			else
				CopyElements( nCount, m_Node.m_Fixed.Data(), other.FixedByType< T >() );
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}

	template < TI K > constexpr void CopyByIndex( I nCount, const CElementsPack &other ) noexcept
	{
		if constexpr ( K == 0 )
			if ( IsOverflow( nCount ) )
				m_Node.m_pData = other.DataByIndex< K, Type >();
			else
				CopyElements( nCount, m_Node.m_Fixed.Data(), other.FixedByIndex< K, Type >() );
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

	template < typename T > constexpr void SwapByType( I nCount, CElementsPack &other ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			if ( IsOverflow( nCount ) )
				Math_Swap( m_Node.m_pData, other.m_Node.m_pData );
			else
				Math_Swap( m_Node.m_Fixed, other.m_Node.m_Fixed );
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}

	template < TI K > constexpr void SwapByIndex( I nCount, CElementsPack &other ) noexcept
	{
		if constexpr ( K == 0 )
			if ( IsOverflow( nCount ) )
				Math_Swap( m_Node.m_pData, other.m_Node.m_pData );
			else
				Math_Swap( m_Node.m_Fixed, other.m_Node.m_Fixed );
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

protected:
	union Node_t
	{
		constexpr Node_t() noexcept : m_pData( nullptr ) {}

		Fixed_t     m_Fixed;
		Data_t      m_pData;
	} m_Node;
};

template < typename I, I N, typename TI, typename T0, typename ...Ts >
class CElementsPack< I, N, TI, T0, Ts... > : public MElementsPack< I, T0 >
{
public:
	using Type = T0;
	using Tail_t = CElementsPack< I, N, TI, Ts... >;
	using Meta_t = MElementsPack< I, T0 >;

	static constexpr I FIXED_COUNT = N > 0 ? N : Meta_t::FIXED_COUNT;

	using Element_t = RemoveCV_t< Type >;
	using Fixed_t = MSelect< FIXED_COUNT == 0 >::template Apply_t
		<
			CEmptyArray< I, Element_t >,
			CArray< I, Element_t, FIXED_COUNT >
		>
	;
	using Data_t = Type *;

	constexpr CElementsPack() noexcept : m_Node() {};

	static constexpr bool IsOverflow( I nCount ) noexcept { return nCount >= FIXED_COUNT; }
	template < typename T > constexpr bool IsOverflowByType( I nCount ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsOverflow( nCount );
		else
			return m_Tail.template IsOverflowByType< T >( nCount );
	}
	template < TI K > constexpr bool IsOverflowByIndex( I nCount ) const noexcept
	{
		if constexpr ( K == 0 )
			return IsOverflow( nCount );
		else
			return m_Tail.template IsOverflowByIndex< K - 1 >( nCount );
	}

	// ---- static storage access (SoA arrays), returns T* ----
	template < typename T > RemoveCV_t< T > *FixedByType() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_Fixed.Data();
		else
			return m_Tail.template FixedByType< T >();
	}
	template < typename T > const T *FixedByType() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_Fixed.Data();
		else
			return m_Tail.template FixedByType< T >();
	}
	template < TI K, typename T > RemoveCV_t< T > *FixedByIndex() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_Fixed.Data();
		else
			return m_Tail.template FixedByIndex< K - 1, T >();
	}
	template < TI K, typename T > const T *FixedByIndex() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_Fixed.Data();
		else
			return m_Tail.template FixedByIndex< K - 1, T >();
	}

	// ---- pointer storage access (AoS of pointers), returns T* ----
	template < typename T > Data_t &DataByType() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pData;
		else
			return m_Tail.template DataByType< T >();
	}
	template < typename T > const Data_t &DataByType() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pData;
		else
			return m_Tail.template DataByType< T >();
	}
	template < TI K, typename T > Data_t &DataByIndex() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pData;
		else
			return m_Tail.template DataByIndex< K - 1, T >();
	}
	template < TI K, typename T > const Data_t &DataByIndex() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pData;
		else
			return m_Tail.template DataByIndex< K - 1, T >();
	}

	template < typename T > T *ByType( bool bOverflow ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return bOverflow ? DataByType< T >() : FixedByType< T >();
		else
			return m_Tail.template ByType< T >();
	}
	template < typename T > const T *ByType( bool bOverflow ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return bOverflow ? DataByType< T >() : FixedByType< T >();
		else
			return m_Tail.template ByType< T >();
	}

	template < TI K, typename T > T *ByIndex( bool bOverflow ) noexcept
	{
		if constexpr ( K == 0 )
			return bOverflow ? DataByIndex< K, T >() : FixedByIndex< K, T >();
		else
			return m_Tail.template ByIndex< K - 1, T >();
	}
	template < TI K, typename T > const T *ByIndex( bool bOverflow ) const noexcept
	{
		if constexpr ( K == 0 )
			return bOverflow ? DataByIndex< K, T >() : FixedByIndex< K, T >();
		else
			return m_Tail.template ByIndex< K - 1, T >();
	}

	template < typename T > constexpr void CopyByType( I nCount, const CElementsPack &other ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			if ( IsOverflow( nCount ) )
				m_Node.m_pData = other.DataByType< T >();
			else
				CopyElements( nCount, m_Node.m_Fixed.Data(), other.FixedByType< T >() );
		else
			m_Tail.template CopyByType< T >( other.m_Tail );
	}

	template < TI K > constexpr void CopyByIndex( I nCount, const CElementsPack &other ) noexcept
	{
		if constexpr ( K == 0 )
			if ( IsOverflow( nCount ) )
				m_Node.m_pData = other.DataByIndex< K, Type >();
			else
				CopyElements( nCount, m_Node.m_Fixed.Data(), other.FixedByIndex< K, Type >() );
		else
			m_Tail.template CopyByIndex< K - 1 >( other.m_Tail );
	}

	template < typename T > constexpr void SwapByType( I nCount, CElementsPack &other ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			if ( IsOverflow( nCount ) )
				Math_Swap( m_Node.m_pData, other.m_Node.m_pData );
			else
				Math_Swap( m_Node.m_Fixed, other.m_Node.m_Fixed );
		else
			m_Tail.template SwapByType< T >( other.m_Tail );
	}

	template < TI K > constexpr void SwapByIndex( I nCount, CElementsPack &other ) noexcept
	{
		if constexpr ( K == 0 )
			if ( IsOverflow( nCount ) )
				Math_Swap( m_Node.m_pData, other.m_Node.m_pData );
			else
				Math_Swap( m_Node.m_Fixed, other.m_Node.m_Fixed );
		else
			m_Tail.template SwapByIndex< K - 1 >( other.m_Tail );
	}

protected:
	union Node_t
	{
		constexpr Node_t() noexcept : m_pData( nullptr ) {}

		Fixed_t     m_Fixed;
		Data_t      m_pData;
	} m_Node;
	Tail_t m_Tail;
};

#endif // !defined( _INCLUDE_BALL_TYPES_ELEMENTSPACK_HPP_ )
