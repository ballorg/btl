#ifndef _INCLUDE_BALL_TYPES_META_PACK_HPP_
#	define _INCLUDE_BALL_TYPES_META_PACK_HPP_

#	pragma once

#	include "xvalue.hpp"
#	include "issame.hpp"

///-----------------------------------------------------------------------------
/// @brief Compile-time value pack of Ts...
///        MPack< size_t, A, B, C > stores values A, B, C
///
/// API:
///   - BaseBy< T >()        -> value by type (T must be unique in Ts...)
///   - BaseBy< K >()        -> value by index (0..N-1)
///   - Reset()              -> value-initialize all nodes
///   - Swap( other )        -> swap pointers
///   - CopyFrom( other )    -> copy values
///   - MoveFrom( other )    -> move values
///-----------------------------------------------------------------------------
template < typename TI, typename ...Ts >
class MPack;

// final pack
template < typename TI, typename T0 >
class MPack< TI, T0 >
{
public:
	using Type = T0;

	constexpr MPack() noexcept : m_Node() {}
	template < typename U0 >
	explicit constexpr MPack( U0 &&value ) noexcept : m_Node( Forward< U0 >( value ) ) {}

	constexpr MPack( const MPack &copyFrom ) noexcept : MPack() { CopyFrom( copyFrom ); }
	constexpr MPack( MPack &&moveFrom ) noexcept { MoveFrom( Move( moveFrom ) ); }
	constexpr MPack &operator=( const MPack &copyFrom ) noexcept { return CopyFrom( copyFrom ); }
	constexpr MPack &operator=( MPack &&moveFrom ) noexcept { return MoveFrom( moveFrom ); }

	// type access (T must be unique within Ts...)
	template < typename T > constexpr T &BaseBy() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node;
		else
			static_assert( IS_SAME< Type, T >, "MPack: typed OOB for empty pack" );
	}
	template < typename T > constexpr const T &BaseBy() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node;
		else
			static_assert( IS_SAME< Type, T >, "MPack: index OOB for empty pack" );
	}

	// index access
	template < TI K > constexpr decltype( auto ) BaseBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node;
		else
			static_assert( K == 0, "MPack: index OOB for empty pack" );
	}
	template < TI K > constexpr decltype( auto ) BaseBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node;
		else
			static_assert( K == 0, "MPack: index OOB for empty pack" );
	}

	// utilities
	constexpr void Reset() noexcept
	{
		m_Node = T0();
	}

public:
	constexpr void Swap( MPack &other ) noexcept
	{
		T0 temp( Move( m_Node ) );
		m_Node = Move( other.m_Node );
		other.m_Node = Move( temp );
	}

	constexpr MPack &CopyFrom( const MPack &other ) noexcept
	{
		m_Node = other.m_Node;

		return *this;
	}

	constexpr MPack &MoveFrom( MPack &&other ) noexcept
	{
		Swap( other );

		return *this;
	}

public:
	T0 m_Node;
};

// non-empty pack
template < typename TI, typename T0, typename ...Ts >
class MPack< TI, T0, Ts... >
{
public:
	using Type = T0;
	using Tail_t = MPack< TI, Ts... >;

	constexpr MPack() noexcept : m_Node(), m_Tail() {}
	template < typename U0, typename ...Us >
	explicit constexpr MPack( U0 &&value, Us &&...tail ) noexcept : m_Node( Forward< U0 >( value ) ), m_Tail( Forward< Us >( tail )... ) {}

	constexpr MPack( const MPack &copyFrom ) noexcept : MPack() { CopyFrom( copyFrom ); }
	constexpr MPack( MPack &&moveFrom ) noexcept { MoveFrom( Move( moveFrom ) ); }
	constexpr MPack &operator=( const MPack &copyFrom ) noexcept { return CopyFrom( copyFrom ); }
	constexpr MPack &operator=( MPack &&moveFrom ) noexcept { return MoveFrom( moveFrom ); }

	// type access (T must be unique within Ts...)
	template < typename T > constexpr T &BaseBy() noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node;
		else
			return m_Tail.template BaseBy< T >();
	}
	template < typename T > constexpr const T &BaseBy() const noexcept
	{
		if constexpr ( IS_SAME< Type, T > )
			return m_Node;
		else
			return m_Tail.template BaseBy< T >();
	}

	// index access
	template < TI K > constexpr decltype( auto ) BaseBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node;
		else
			return m_Tail.template BaseBy< K - 1 >();
	}
	template < TI K > constexpr decltype( auto ) BaseBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node;
		else
			return m_Tail.template BaseBy< K - 1 >();
	}

	// utilities
	constexpr void Reset() noexcept
	{
		m_Node = T0();
		m_Tail.Reset();
	}

public:
	constexpr void Swap( MPack &other ) noexcept
	{
		T0 temp( Move( m_Node ) );
		m_Node = Move( other.m_Node );
		other.m_Node = Move( temp );
		m_Tail.Swap( other.m_Tail );
	}

	constexpr MPack &CopyFrom( const MPack &other ) noexcept
	{
		m_Node = other.m_Node;
		m_Tail.CopyFrom( other.m_Tail );

		return *this;
	}

	constexpr MPack &MoveFrom( MPack &&other ) noexcept
	{
		Swap( other );

		return *this;
	}

public:
	T0 m_Node;
	Tail_t m_Tail;
};

template < typename TI, typename ...Ts >
class MPointerPack;

template < typename TI, typename T0 >
class MPointerPack< TI, T0 > : public MPack< TI, T0 * >
{
public:
	using Base_t = MPack< TI, T0 * >;
	using Base_t::Base_t;
};

template < typename TI, typename T0, typename ...Ts >
class MPointerPack< TI, T0, Ts... > : public MPack< TI, T0 *, Ts *... >
{
public:
	using Base_t = MPack< TI, T0 *, Ts *... >;
	using Base_t::Base_t;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_PACK_HPP_ )
