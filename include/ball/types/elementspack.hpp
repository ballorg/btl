#ifndef _INCLUDE_BALL_TYPES_ELEMENTSPACK_HPP_
#	define _INCLUDE_BALL_TYPES_ELEMENTSPACK_HPP_

#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "meta/pack.hpp"
#	include "meta/removecv.hpp"
#	include "meta/select.hpp"
#	include "array.hpp"
#	include "elements.hpp"
#	include "fixed.hpp"

template < typename I, typename T >
struct MElementsPackBase
{
	using Type = T;

	static constexpr size_t FIXED_ELEMENT_SIZE = sizeof( Type );
	static constexpr size_t POINTER_SIZE = sizeof( Type * );
	static constexpr bool   FIXED_OVERFLOWS_POINTER = POINTER_SIZE >= FIXED_ELEMENT_SIZE;
	static constexpr I      FIXED_COUNT = I( POINTER_SIZE / FIXED_ELEMENT_SIZE );
};

template < typename I, I N, typename T >
struct MElementsPack : MElementsPackBase< I, T >
{
	using Type = T;
	using Base_t = MElementsPackBase< I, T >;

	static constexpr I FIXED_COUNT = N > 0 ? N : Base_t::FIXED_COUNT;
	static constexpr size_t FIXED_SIZE = FIXED_COUNT * Base_t::FIXED_ELEMENT_SIZE;

	using Element_t = RemoveCV_t< Type >;
	using Fixed_t = MSelect< FIXED_COUNT == 0 >::template Apply_t
		<
			CEmptyArray< I, Element_t >,
			CArray< I, Element_t, FIXED_COUNT >
		>
	;
	using Data_t = Type *;

	using Packed_t = uchar_t;
	using PackedFixed_t = MSelect< FIXED_COUNT == 0 >::template Apply_t
		<
			CEmptyArray< I, Packed_t >,
			CArray< I, Packed_t, FIXED_SIZE >
		>
	;
	using PackedData_t = Packed_t *;

	static constexpr bool IsOverflow( I nCount ) noexcept { return nCount >= FIXED_COUNT; }

	static constexpr size_t PackedBytesForCount( I nCount ) noexcept
	{
		const bits_t nBits = static_cast< bits_t >( nCount ) * MFixedMetadata< Element_t >::BITS;

		return static_cast< size_t >( ( nBits + bits_t( 7 ) ) / bits_t( 8 ) );
	}
	static constexpr bool IsPackedOverflow( I nCount ) noexcept { return PackedBytesForCount( nCount ) > FIXED_SIZE; }
};

template < typename I, I N, typename TI, typename ...Ts >
class CElementsPack;

template < typename I, I N, typename TI, typename T0 >
class CElementsPack< I, N, TI, T0 > : public MElementsPack< I, N, T0 >
{
public:
	using Type = T0;
	using Base_t = MElementsPack< I, N, T0 >;
	using Base_t::FIXED_COUNT;
	using Base_t::FIXED_SIZE;
	using typename Base_t::Element_t;
	using typename Base_t::Fixed_t;
	using typename Base_t::Packed_t;
	using typename Base_t::PackedFixed_t;
	using typename Base_t::Data_t;
	using typename Base_t::PackedData_t;
	using Base_t::IsOverflow;
	using Base_t::IsPackedOverflow;

	constexpr CElementsPack() noexcept : m_Node() {}

	static constexpr bool IsOverflow( I nCount ) noexcept { return nCount > FIXED_COUNT; }
	template < typename T > constexpr bool IsOverflowBy( I nCount ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsOverflow( nCount );
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K > constexpr bool IsOverflowBy( I nCount ) const noexcept
	{
		if constexpr ( K == 0 )
			return IsOverflow( nCount );
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

	template < typename T > constexpr bool IsPackedOverflowBy( I nCount ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsPackedOverflow( nCount );
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K > constexpr bool IsPackedOverflowBy( I nCount ) const noexcept
	{
		if constexpr ( K == 0 )
			return IsPackedOverflow( nCount );
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

	// ---- static storage access (SoA arrays), returns T* ----
	template < typename T > constexpr RemoveCV_t< T > *FixedBy() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_Fixed.Data();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr const T *FixedBy() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_Fixed.Data();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr RemoveCV_t< T > *FixedBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_Fixed.Data();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr const T *FixedBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_Fixed.Data();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr Packed_t *PackedFixedBy() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_PackedFixed.Data();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr const Packed_t *PackedFixedBy() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_PackedFixed.Data();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr Packed_t *PackedFixedBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_PackedFixed.Data();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr const Packed_t *PackedFixedBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_PackedFixed.Data();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

	// ---- pointer storage access (AoS of pointers), returns T* ----
	template < typename T > constexpr Data_t &DataBy() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pData;
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr const Data_t &DataBy() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pData;
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr Data_t &DataBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pData;
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr const Data_t &DataBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pData;
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr Packed_t *&PackedDataBy() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pPackedData;
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr Packed_t *const &PackedDataBy() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pPackedData;
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr Packed_t *&PackedDataBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pPackedData;
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr Packed_t *const &PackedDataBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pPackedData;
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

	template < typename T > constexpr T *BaseBy( I nCount ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsOverflow( nCount ) ? DataBy< T >() : FixedBy< T >();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr const T *BaseBy( I nCount ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsOverflow( nCount ) ? DataBy< T >() : FixedBy< T >();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}

	template < TI K, typename T > constexpr T *BaseBy( I nCount ) noexcept
	{
		if constexpr ( K == 0 )
			return IsOverflow( nCount ) ? DataBy< K, T >() : FixedBy< K, T >();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > const T *BaseBy( I nCount ) const noexcept
	{
		if constexpr ( K == 0 )
			return IsOverflow( nCount ) ? DataBy< K, T >() : FixedBy< K, T >();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr Packed_t *PackedBaseBy( I nCount ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsPackedOverflow( nCount ) ? PackedDataBy< T >() : PackedFixedBy< T >();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr const Packed_t *PackedBaseBy( I nCount ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsPackedOverflow( nCount ) ? PackedDataBy< T >() : PackedFixedBy< T >();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr Packed_t *PackedBaseBy( bool bOverflow ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return bOverflow ? PackedDataBy< T >() : PackedFixedBy< T >();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr const Packed_t *PackedBaseBy( bool bOverflow ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return bOverflow ? PackedDataBy< T >() : PackedFixedBy< T >();
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr Packed_t *PackedBaseBy( I nCount ) noexcept
	{
		if constexpr ( K == 0 )
			return IsPackedOverflow( nCount ) ? PackedDataBy< K, T >() : PackedFixedBy< K, T >();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr const Packed_t *PackedBaseBy( I nCount ) const noexcept
	{
		if constexpr ( K == 0 )
			return IsPackedOverflow( nCount ) ? PackedDataBy< K, T >() : PackedFixedBy< K, T >();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr Packed_t *PackedBaseBy( bool bOverflow ) noexcept
	{
		if constexpr ( K == 0 )
			return bOverflow ? PackedDataBy< K, T >() : PackedFixedBy< K, T >();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}
	template < TI K, typename T > constexpr const Packed_t *PackedBaseBy( bool bOverflow ) const noexcept
	{
		if constexpr ( K == 0 )
			return bOverflow ? PackedDataBy< K, T >() : PackedFixedBy< K, T >();
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

	template < typename T > constexpr void CopyBy( I nCount, const CElementsPack &other ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			if ( IsOverflow( nCount ) )
				m_Node.m_pData = other.DataBy< T >();
			else
				CopyElements_Unified( nCount, m_Node.m_Fixed.Data(), other.FixedBy< T >() );
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}

	template < TI K > constexpr void CopyBy( I nCount, const CElementsPack &other ) noexcept
	{
		if constexpr ( K == 0 )
			if ( IsOverflow( nCount ) )
				m_Node.m_pData = other.DataBy< K, Type >();
			else
				CopyElements_Unified( nCount, m_Node.m_Fixed.Data(), other.FixedBy< K, Type >() );
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

	template < typename T > constexpr void SwapBy( I nCount, CElementsPack &other ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			if ( IsOverflow( nCount ) )
				Swap( m_Node.m_pData, other.m_Node.m_pData );
			else
				Swap( m_Node.m_Fixed, other.m_Node.m_Fixed );
		else
			static_assert( IS_SAME< Type, T >, "CElementsPack: typed OOB for empty pack" );
	}

	template < TI K > constexpr void SwapBy( I nCount, CElementsPack &other ) noexcept
	{
		if constexpr ( K == 0 )
			if ( IsOverflow( nCount ) )
				Swap( m_Node.m_pData, other.m_Node.m_pData );
			else
				Swap( m_Node.m_Fixed, other.m_Node.m_Fixed );
		else
			static_assert( K == 0, "CElementsPack: typed OOB for empty pack" );
	}

protected:
	union Node_t
	{
		constexpr Node_t() noexcept : m_pData( nullptr ) {}

		Fixed_t         m_Fixed;
		Data_t          m_pData;
		PackedFixed_t   m_PackedFixed;
		PackedData_t    m_pPackedData;
	} m_Node;
};

template < typename I, I N, typename TI, typename T0, typename ...Ts >
class CElementsPack< I, N, TI, T0, Ts... > : public MElementsPack< I, N, T0 >
{
public:
	using Type = T0;
	using Base_t = MElementsPack< I, N, T0 >;
	using Tail_t = CElementsPack< I, N, TI, Ts... >;
	using Base_t::FIXED_COUNT;
	using Base_t::FIXED_SIZE;
	using typename Base_t::Element_t;
	using typename Base_t::Fixed_t;
	using typename Base_t::Packed_t;
	using typename Base_t::PackedFixed_t;
	using typename Base_t::Data_t;
	using typename Base_t::PackedData_t;
	using Base_t::IsOverflow;
	using Base_t::IsPackedOverflow;

	constexpr CElementsPack() noexcept : m_Node() {};

	static constexpr bool IsOverflow( I nCount ) noexcept { return nCount >= FIXED_COUNT; }
	template < typename T > constexpr bool IsOverflowBy( I nCount ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsOverflow( nCount );
		else
			return m_Tail.template IsOverflowBy< T >( nCount );
	}
	template < TI K > constexpr bool IsOverflowBy( I nCount ) const noexcept
	{
		if constexpr ( K == 0 )
			return IsOverflow( nCount );
		else
			return m_Tail.template IsOverflowBy< K - 1 >( nCount );
	}
	template < typename T > constexpr bool IsPackedOverflowBy( I nCount ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return IsPackedOverflow( nCount );
		else
			return m_Tail.template IsPackedOverflowBy< T >( nCount );
	}
	template < TI K > constexpr bool IsPackedOverflowBy( I nCount ) const noexcept
	{
		if constexpr ( K == 0 )
			return IsPackedOverflow( nCount );
		else
			return m_Tail.template IsPackedOverflowBy< K - 1 >( nCount );
	}

	// ---- static storage access (SoA arrays), returns T* ----
	template < typename T > constexpr RemoveCV_t< T > *FixedBy() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_Fixed.Data();
		else
			return m_Tail.template FixedBy< T >();
	}
	template < typename T > constexpr const T *FixedBy() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_Fixed.Data();
		else
			return m_Tail.template FixedBy< T >();
	}
	template < TI K, typename T > constexpr RemoveCV_t< T > *FixedBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_Fixed.Data();
		else
			return m_Tail.template FixedBy< K - 1, T >();
	}
	template < TI K, typename T > constexpr const T *FixedBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_Fixed.Data();
		else
			return m_Tail.template FixedBy< K - 1, T >();
	}
	template < typename T > constexpr Packed_t *PackedFixedBy() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_PackedFixed.Data();
		else
			return m_Tail.template PackedFixedBy< T >();
	}
	template < typename T > constexpr const Packed_t *PackedFixedBy() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_PackedFixed.Data();
		else
			return m_Tail.template PackedFixedBy< T >();
	}
	template < TI K, typename T > constexpr Packed_t *PackedFixedBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_PackedFixed.Data();
		else
			return m_Tail.template PackedFixedBy< K - 1, T >();
	}
	template < TI K, typename T > constexpr const Packed_t *PackedFixedBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_PackedFixed.Data();
		else
			return m_Tail.template PackedFixedBy< K - 1, T >();
	}

	// ---- pointer storage access (AoS of pointers), returns T* ----
	template < typename T > constexpr auto &DataBy() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pData;
		else
			return m_Tail.template DataBy< T >();
	}
	template < typename T > constexpr const auto &DataBy() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pData;
		else
			return m_Tail.template DataBy< T >();
	}
	template < TI K, typename T > constexpr auto &DataBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pData;
		else
			return m_Tail.template DataBy< K - 1, T >();
	}
	template < TI K, typename T > constexpr const auto &DataBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pData;
		else
			return m_Tail.template DataBy< K - 1, T >();
	}
	template < typename T > constexpr auto &PackedDataBy() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pPackedData;
		else
			return m_Tail.template PackedDataBy< T >();
	}
	template < typename T > constexpr const auto &PackedDataBy() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node.m_pPackedData;
		else
			return m_Tail.template PackedDataBy< T >();
	}
	template < TI K, typename T > constexpr auto &PackedDataBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pPackedData;
		else
			return m_Tail.template PackedDataBy< K - 1, T >();
	}
	template < TI K, typename T > constexpr const auto &PackedDataBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node.m_pPackedData;
		else
			return m_Tail.template PackedDataBy< K - 1, T >();
	}

	template < typename T > constexpr T *BaseBy( bool bOverflow ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return bOverflow ? DataBy< T >() : FixedBy< T >();
		else
			return m_Tail.template BaseBy< T >( bOverflow );
	}
	template < typename T > constexpr const T *BaseBy( bool bOverflow ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return bOverflow ? DataBy< T >() : FixedBy< T >();
		else
			return m_Tail.template BaseBy< T >( bOverflow );
	}

	template < TI K, typename T > constexpr T *BaseBy( bool bOverflow ) noexcept
	{
		if constexpr ( K == 0 )
			return bOverflow ? DataBy< K, T >() : FixedBy< K, T >();
		else
			return m_Tail.template BaseBy< K - 1, T >( bOverflow );
	}
	template < TI K, typename T > constexpr const T *BaseBy( bool bOverflow ) const noexcept
	{
		if constexpr ( K == 0 )
			return bOverflow ? DataBy< K, T >() : FixedBy< K, T >();
		else
			return m_Tail.template BaseBy< K - 1, T >( bOverflow );
	}
	template < typename T > constexpr Packed_t *PackedBaseBy( I nCount ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return PackedBaseBy< T >( IsPackedOverflow( nCount ) );
		else
			return m_Tail.template PackedBaseBy< T >( nCount );
	}
	template < typename T > constexpr const Packed_t *PackedBaseBy( I nCount ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return PackedBaseBy< T >( IsPackedOverflow( nCount ) );
		else
			return m_Tail.template PackedBaseBy< T >( nCount );
	}
	template < typename T > constexpr Packed_t *PackedBaseBy( bool bOverflow ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return bOverflow ? PackedDataBy< T >() : PackedFixedBy< T >();
		else
			return m_Tail.template PackedBaseBy< T >( bOverflow );
	}
	template < typename T > constexpr const Packed_t *PackedBaseBy( bool bOverflow ) const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return bOverflow ? PackedDataBy< T >() : PackedFixedBy< T >();
		else
			return m_Tail.template PackedBaseBy< T >( bOverflow );
	}
	template < TI K, typename T > constexpr Packed_t *PackedBaseBy( I nCount ) noexcept
	{
		if constexpr ( K == 0 )
			return PackedBaseBy< K, T >( IsPackedOverflow( nCount ) );
		else
			return m_Tail.template PackedBaseBy< K - 1, T >( nCount );
	}
	template < TI K, typename T > constexpr const Packed_t *PackedBaseBy( I nCount ) const noexcept
	{
		if constexpr ( K == 0 )
			return PackedBaseBy< K, T >( IsPackedOverflow( nCount ) );
		else
			return m_Tail.template PackedBaseBy< K - 1, T >( nCount );
	}
	template < TI K, typename T > constexpr Packed_t *PackedBaseBy( bool bOverflow ) noexcept
	{
		if constexpr ( K == 0 )
			return bOverflow ? PackedDataBy< K, T >() : PackedFixedBy< K, T >();
		else
			return m_Tail.template PackedBaseBy< K - 1, T >( bOverflow );
	}
	template < TI K, typename T > constexpr const Packed_t *PackedBaseBy( bool bOverflow ) const noexcept
	{
		if constexpr ( K == 0 )
			return bOverflow ? PackedDataBy< K, T >() : PackedFixedBy< K, T >();
		else
			return m_Tail.template PackedBaseBy< K - 1, T >( bOverflow );
	}

	template < typename T > constexpr void CopyBy( I nCount, const CElementsPack &other ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			if ( IsOverflow( nCount ) )
				m_Node.m_pData = other.DataBy< T >();
			else
				CopyElements_Unified( nCount, m_Node.m_Fixed.Data(), other.FixedBy< T >() );
		else
			m_Tail.template CopyBy< T >( nCount, other.m_Tail );
	}

	template < TI K > constexpr void CopyBy( I nCount, const CElementsPack &other ) noexcept
	{
		if constexpr ( K == 0 )
			if ( IsOverflow( nCount ) )
				m_Node.m_pData = other.DataBy< K, Type >();
			else
				CopyElements_Unified( nCount, m_Node.m_Fixed.Data(), other.FixedBy< K, Type >() );
		else
			m_Tail.template CopyBy< K - 1 >( nCount, other.m_Tail );
	}

	template < typename T > constexpr void SwapBy( I nCount, CElementsPack &other ) noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			if ( IsOverflow( nCount ) )
				Swap( m_Node.m_pData, other.m_Node.m_pData );
			else
				Swap( m_Node.m_Fixed, other.m_Node.m_Fixed );
		else
			m_Tail.template SwapBy< T >( nCount, other.m_Tail );
	}

	template < TI K > constexpr void SwapBy( I nCount, CElementsPack &other ) noexcept
	{
		if constexpr ( K == 0 )
			if ( IsOverflow( nCount ) )
				Swap( m_Node.m_pData, other.m_Node.m_pData );
			else
				Swap( m_Node.m_Fixed, other.m_Node.m_Fixed );
		else
			m_Tail.template SwapBy< K - 1 >( nCount, other.m_Tail );
	}

protected:
	union Node_t
	{
		constexpr Node_t() noexcept : m_pData( nullptr ) {}

		Fixed_t         m_Fixed;
		Data_t          m_pData;
		PackedFixed_t   m_PackedFixed;
		PackedData_t    m_pPackedData;
	} m_Node;
	Tail_t m_Tail;
};

#endif // !defined( _INCLUDE_BALL_TYPES_ELEMENTSPACK_HPP_ )
