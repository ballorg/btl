#ifndef _INCLUDE_BALL_TYPES_ELEMENTSPACK_HPP_
#	define _INCLUDE_BALL_TYPES_ELEMENTSPACK_HPP_

#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/assert/static.h"
#	include "meta/constevaluated.hpp"
#	include "meta/indextype.hpp"
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
	static constexpr bool FIXED_OVERFLOWS_POINTER = POINTER_SIZE >= FIXED_ELEMENT_SIZE;
	static constexpr I FIXED_COUNT = I( POINTER_SIZE / FIXED_ELEMENT_SIZE );
};

template < typename I, I N, typename T >
struct MElementsPack : MElementsPackBase< I, T >
{
	using Type = T;
	using Base_t = MElementsPackBase< I, T >;

	static constexpr I FIXED_COUNT = N > 0 ? N : Base_t::FIXED_COUNT;
	static constexpr size_t FIXED_SIZE = FIXED_COUNT * Base_t::FIXED_ELEMENT_SIZE;

	using Element_t = RemoveCV_t< Type >;
	using Traits_t = MFixedMetadata< Element_t >;
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

	static constexpr bool IS_PACKED = Traits_t::IS_PACKED;
	static constexpr bits_t PACKED_BITS = Traits_t::BITS;
	static constexpr I STORAGE_FIXED_COUNT = IS_PACKED ? static_cast< I >( ( FIXED_SIZE * size_t( 8 ) ) / static_cast< size_t >( PACKED_BITS ) ) : FIXED_COUNT;

	static constexpr bool IsOverflow( I nCount ) noexcept { return nCount > FIXED_COUNT; }

	static constexpr size_t PackedSize( I nCount ) noexcept
	{
		const bits_t nBits = static_cast< bits_t >( nCount ) * MFixedMetadata< Element_t >::BITS;

		return static_cast< size_t >( ( nBits + 7 ) / 8 );
	}
	static constexpr bool IsPackedOverflow( I nCount ) noexcept { return PackedSize( nCount ) > FIXED_SIZE; }
};

/// Per-column inline/external storage node used by CElementsPack.
template < typename I, I N, typename T >
class CElementsNode : public MElementsPack< I, N, T >
{
public:
	using Type = T;
	using Base_t = MElementsPack< I, N, T >;
	using typename Base_t::Element_t;
	using typename Base_t::Fixed_t;
	using typename Base_t::Packed_t;
	using typename Base_t::PackedFixed_t;
	using typename Base_t::Data_t;
	using typename Base_t::PackedData_t;
	using Base_t::FIXED_COUNT;

	/// @complexity O(1): selects the pointer member or initializes one inline value.
	constexpr CElementsNode() noexcept : m_Node() {}
	constexpr CElementsNode( const Type &value ) noexcept : m_Node( value ) {}
	constexpr CElementsNode( Type &&value ) noexcept : m_Node( Move( value ) ) {}

	/// @complexity O(1): direct union-member access.
	constexpr RemoveCV_t< Type > *Fixed() noexcept { return m_Node.m_Fixed.Data(); }
	constexpr const Type *Fixed() const noexcept { return m_Node.m_Fixed.Data(); }
	constexpr Packed_t *PackedFixed() noexcept { return m_Node.m_PackedFixed.Data(); }
	constexpr const Packed_t *PackedFixed() const noexcept { return m_Node.m_PackedFixed.Data(); }
	constexpr Data_t &Data() noexcept { return m_Node.m_pData; }
	constexpr const Data_t &Data() const noexcept { return m_Node.m_pData; }
	constexpr PackedData_t &PackedData() noexcept { return m_Node.m_pPackedData; }
	constexpr const PackedData_t &PackedData() const noexcept { return m_Node.m_pPackedData; }

	/// @complexity O(1): selects one of two storage members.
	constexpr Type *Base( bool bOverflow ) noexcept { return bOverflow ? Data() : Fixed(); }
	constexpr const Type *Base( bool bOverflow ) const noexcept { return bOverflow ? Data() : Fixed(); }
	constexpr Packed_t *PackedBase( bool bOverflow ) noexcept { return bOverflow ? PackedData() : PackedFixed(); }
	constexpr const Packed_t *PackedBase( bool bOverflow ) const noexcept { return bOverflow ? PackedData() : PackedFixed(); }

	/// @complexity O(nCount): copies the requested inline elements.
	template < typename U > constexpr void StoreFixed( I nCount, const U *pSrc ) noexcept
	{
		if ( IsConstantEvaluated() )
		{
			m_Node.m_Fixed = Fixed_t();
			CopyElements_Unified( nCount, m_Node.m_Fixed.Data(), pSrc );
		}
		else
		{
			CopyElements( nCount, m_Node.m_Fixed.Data(), pSrc );
		}
	}

	/// @complexity O(FIXED_SIZE) during constant evaluation; O(1) at run time.
	constexpr void ActivatePackedFixed() noexcept
	{
		if ( IsConstantEvaluated() )
			m_Node.m_PackedFixed = PackedFixed_t();
	}

	/// @complexity O(nCount) for inline storage; O(1) for pointer storage.
	constexpr void Copy( I nCount, const CElementsNode &other, bool bOverflow ) noexcept
	{
		if ( bOverflow )
			m_Node.m_pData = other.m_Node.m_pData;
		else
			CopyElements_Unified( nCount, m_Node.m_Fixed.Data(), other.m_Node.m_Fixed.Data() );
	}

	/// @complexity O(FIXED_COUNT) for inline storage; O(1) for pointer storage.
	constexpr void SwapWith( CElementsNode &other, bool bOverflow ) noexcept
	{
		if ( bOverflow )
			Swap( m_Node.m_pData, other.m_Node.m_pData );
		else
			Swap( m_Node.m_Fixed, other.m_Node.m_Fixed );
	}

protected:
	union Node_t
	{
		/// @complexity O(1), plus O(PACKED_BITS) when encoding a packed value.
		constexpr Node_t() noexcept : m_pData( nullptr ) {}
		constexpr Node_t( const Type &value ) noexcept : m_pData( nullptr )
		{
			BALL_STATIC_ASSERT( FIXED_COUNT > 0, "CElementsPack: a value requires inline storage" );

			if constexpr ( Base_t::IS_PACKED )
				StorePacked( value );
			else
			{
				m_Fixed = Fixed_t();
				m_Fixed[ 0 ] = value;
			}
		}
		constexpr Node_t( Type &&value ) noexcept : m_pData( nullptr )
		{
			BALL_STATIC_ASSERT( FIXED_COUNT > 0, "CElementsPack: a value requires inline storage" );

			if constexpr ( Base_t::IS_PACKED )
				StorePacked( value );
			else
			{
				m_Fixed = Fixed_t();
				m_Fixed[ 0 ] = Move( value );
			}
		}

		/// @complexity O(PACKED_BITS): writes one logical packed value bit by bit.
		constexpr void StorePacked( const Type &value ) noexcept
		{
			using U = typename Base_t::Traits_t::Unsigned_t;

			m_PackedFixed = PackedFixed_t();
			const U nValue = static_cast< U >( value );

			for ( bits_t n = 0; n < Base_t::PACKED_BITS; ++n )
				if ( ( nValue >> n ) & 1 )
					m_PackedFixed[ static_cast< I >( n >> 3 ) ] = static_cast< Packed_t >( m_PackedFixed[ static_cast< I >( n >> 3 ) ] | static_cast< Packed_t >( Packed_t( 1 ) << ( n & 7 ) ) );
		}

		Fixed_t m_Fixed;
		Data_t m_pData;
		PackedFixed_t m_PackedFixed;
		PackedData_t m_pPackedData;
	} m_Node;
};

#define BALL_ELEMENTS_PACK_ACCESSORS( Name, NodeMethod ) \
	template < typename T > constexpr decltype( auto ) Name() noexcept { return NodeBy< T >().NodeMethod(); } \
	template < typename T > constexpr decltype( auto ) Name() const noexcept { return NodeBy< T >().NodeMethod(); } \
	template < TI K, typename T > constexpr decltype( auto ) Name() noexcept { return CheckedNodeBy< K, T >().NodeMethod(); } \
	template < TI K, typename T > constexpr decltype( auto ) Name() const noexcept { return CheckedNodeBy< K, T >().NodeMethod(); }

template < typename I, I N, typename TI, typename ...Ts >
class CElementsPack;

/// Typed pack of per-column inline/external storage nodes.
template < typename I, I N, typename TI, typename T0, typename ...Ts >
class CElementsPack< I, N, TI, T0, Ts... > : public MElementsPack< I, N, T0 >
{
public:
	using Type = T0;
	using Base_t = MElementsPack< I, N, T0 >;
	using Nodes_t = MPack< TI, CElementsNode< I, N, T0 >, CElementsNode< I, N, Ts >... >;
	template < TI K > using NodeByIndex_t = typename MIndexType< TI, K, CElementsNode< I, N, T0 >, CElementsNode< I, N, Ts >... >::Type;
	static constexpr I COMMON_FIXED_COUNT = []() constexpr noexcept
	{
		I nCount = Base_t::STORAGE_FIXED_COUNT;

		( ( nCount = nCount < MElementsPack< I, N, Ts >::STORAGE_FIXED_COUNT ? nCount : MElementsPack< I, N, Ts >::STORAGE_FIXED_COUNT ), ... );

		return nCount;
	}();
	using Base_t::FIXED_COUNT;
	using Base_t::FIXED_SIZE;
	using typename Base_t::Element_t;
	using typename Base_t::Fixed_t;
	using typename Base_t::Packed_t;
	using typename Base_t::PackedFixed_t;
	using typename Base_t::Data_t;
	using typename Base_t::PackedData_t;

	/// @complexity O(sizeof...(Ts)): initializes one node per column.
	constexpr CElementsPack() noexcept : m_Nodes() {}
	explicit constexpr CElementsPack( const Type &value, const Ts &...tail ) noexcept : m_Nodes( value, tail... ) {}
	explicit constexpr CElementsPack( Type &&value, Ts &&...tail ) noexcept : m_Nodes( Move( value ), Move( tail )... ) {}

	/// @complexity O(sizeof...(Ts) * FIXED_COUNT) for inline nodes; O(sizeof...(Ts)) for pointers.
	constexpr CElementsPack( const CElementsPack & ) noexcept = default;
	constexpr CElementsPack( CElementsPack && ) noexcept = default;
	constexpr CElementsPack &operator=( const CElementsPack & ) noexcept = default;
	constexpr CElementsPack &operator=( CElementsPack &&other ) noexcept { m_Nodes.MoveFrom( Move( other.m_Nodes ) ); return *this; }

	/// @complexity O(1): compares against the compile-time shared threshold.
	static constexpr bool IsOverflow( I nCount ) noexcept { return nCount > COMMON_FIXED_COUNT; }
	static constexpr bool IsPackedOverflow( I nCount ) noexcept { return nCount > COMMON_FIXED_COUNT; }
	template < typename T > constexpr bool IsOverflowBy( I nCount ) const noexcept { NodeBy< T >(); return IsOverflow( nCount ); }
	template < TI K > constexpr bool IsOverflowBy( I nCount ) const noexcept { NodeBy< K >(); return IsOverflow( nCount ); }
	template < typename T > constexpr bool IsPackedOverflowBy( I nCount ) const noexcept { NodeBy< T >(); return IsPackedOverflow( nCount ); }
	template < TI K > constexpr bool IsPackedOverflowBy( I nCount ) const noexcept { NodeBy< K >(); return IsPackedOverflow( nCount ); }

	/// @complexity O(k) type-pack traversal, where k is the column index.
	BALL_ELEMENTS_PACK_ACCESSORS( FixedBy, Fixed )
	BALL_ELEMENTS_PACK_ACCESSORS( PackedFixedBy, PackedFixed )
	BALL_ELEMENTS_PACK_ACCESSORS( DataBy, Data )
	BALL_ELEMENTS_PACK_ACCESSORS( PackedDataBy, PackedData )

	/// @complexity O(k) type-pack traversal, where k is the column index.
	template < typename T > constexpr T *BaseBy( I nCount ) noexcept { return NodeBy< T >().Base( IsOverflow( nCount ) ); }
	template < typename T > constexpr const T *BaseBy( I nCount ) const noexcept { return NodeBy< T >().Base( IsOverflow( nCount ) ); }
	template < typename T > constexpr T *BaseBy( bool bOverflow ) noexcept { return NodeBy< T >().Base( bOverflow ); }
	template < typename T > constexpr const T *BaseBy( bool bOverflow ) const noexcept { return NodeBy< T >().Base( bOverflow ); }
	template < TI K, typename T > constexpr T *BaseBy( I nCount ) noexcept { return CheckedNodeBy< K, T >().Base( IsOverflow( nCount ) ); }
	template < TI K, typename T > constexpr const T *BaseBy( I nCount ) const noexcept { return CheckedNodeBy< K, T >().Base( IsOverflow( nCount ) ); }
	template < TI K, typename T > constexpr T *BaseBy( bool bOverflow ) noexcept { return CheckedNodeBy< K, T >().Base( bOverflow ); }
	template < TI K, typename T > constexpr const T *BaseBy( bool bOverflow ) const noexcept { return CheckedNodeBy< K, T >().Base( bOverflow ); }

	/// @complexity O(k) type-pack traversal, where k is the column index.
	template < typename T > constexpr Packed_t *PackedBaseBy( I nCount ) noexcept { return NodeBy< T >().PackedBase( IsPackedOverflow( nCount ) ); }
	template < typename T > constexpr const Packed_t *PackedBaseBy( I nCount ) const noexcept { return NodeBy< T >().PackedBase( IsPackedOverflow( nCount ) ); }
	template < typename T > constexpr Packed_t *PackedBaseBy( bool bOverflow ) noexcept { return NodeBy< T >().PackedBase( bOverflow ); }
	template < typename T > constexpr const Packed_t *PackedBaseBy( bool bOverflow ) const noexcept { return NodeBy< T >().PackedBase( bOverflow ); }
	template < TI K, typename T > constexpr Packed_t *PackedBaseBy( I nCount ) noexcept { return CheckedNodeBy< K, T >().PackedBase( IsPackedOverflow( nCount ) ); }
	template < TI K, typename T > constexpr const Packed_t *PackedBaseBy( I nCount ) const noexcept { return CheckedNodeBy< K, T >().PackedBase( IsPackedOverflow( nCount ) ); }
	template < TI K, typename T > constexpr Packed_t *PackedBaseBy( bool bOverflow ) noexcept { return CheckedNodeBy< K, T >().PackedBase( bOverflow ); }
	template < TI K, typename T > constexpr const Packed_t *PackedBaseBy( bool bOverflow ) const noexcept { return CheckedNodeBy< K, T >().PackedBase( bOverflow ); }

	/// @complexity O(K + nCount) for storage traversal and copying.
	template < TI K, typename T > constexpr void StoreFixedElements( I nCount, const T *pSrc ) noexcept { CheckedNodeBy< K, T >().StoreFixed( nCount, pSrc ); }
	template < TI K > constexpr void ActivatePackedFixed() noexcept { NodeBy< K >().ActivatePackedFixed(); }

	/// @complexity O(k + nCount) for inline storage; O(k) for pointer storage.
	template < typename T > constexpr void CopyBy( I nCount, const CElementsPack &other ) noexcept
	{
		NodeBy< T >().Copy( nCount, other.template NodeBy< T >(), IsOverflow( nCount ) );
	}
	template < TI K > constexpr void CopyBy( I nCount, const CElementsPack &other ) noexcept
	{
		NodeBy< K >().Copy( nCount, other.template NodeBy< K >(), IsOverflow( nCount ) );
	}
	template < typename T > constexpr void SwapBy( I nCount, CElementsPack &other ) noexcept
	{
		NodeBy< T >().SwapWith( other.template NodeBy< T >(), IsOverflow( nCount ) );
	}
	template < TI K > constexpr void SwapBy( I nCount, CElementsPack &other ) noexcept
	{
		NodeBy< K >().SwapWith( other.template NodeBy< K >(), IsOverflow( nCount ) );
	}

private:
	template < typename T > constexpr auto &NodeBy() noexcept { return m_Nodes.template BaseBy< CElementsNode< I, N, T > >(); }
	template < typename T > constexpr const auto &NodeBy() const noexcept { return m_Nodes.template BaseBy< CElementsNode< I, N, T > >(); }
	template < TI K > constexpr auto &NodeBy() noexcept { return m_Nodes.template BaseBy< NodeByIndex_t< K > >(); }
	template < TI K > constexpr const auto &NodeBy() const noexcept { return m_Nodes.template BaseBy< NodeByIndex_t< K > >(); }
	template < TI K, typename T > constexpr decltype( auto ) CheckedNodeBy() noexcept
	{
		BALL_STATIC_ASSERT( ( IS_SAME< typename NodeByIndex_t< K >::Type, T > ), "CElementsPack: type does not match column index" );

		return NodeBy< K >();
	}
	template < TI K, typename T > constexpr decltype( auto ) CheckedNodeBy() const noexcept
	{
		BALL_STATIC_ASSERT( ( IS_SAME< typename NodeByIndex_t< K >::Type, T > ), "CElementsPack: type does not match column index" );

		return NodeBy< K >();
	}

	Nodes_t m_Nodes;
};

#undef BALL_ELEMENTS_PACK_ACCESSORS

#endif // !defined( _INCLUDE_BALL_TYPES_ELEMENTSPACK_HPP_ )
