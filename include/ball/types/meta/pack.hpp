#ifndef _INCLUDE_BALL_TYPES_META_PACK_HPP_
#	define _INCLUDE_BALL_TYPES_META_PACK_HPP_

#	pragma once

#	include "issame.hpp"

///-----------------------------------------------------------------------------
/// @brief Compile-time pack of pointers to types Ts...
///        MPack< A, B, C >  stores: A, B, C
///
/// API:
///   - BaseBy< T >()     -> pointer by type (T must be unique in Ts...)
///   - BaseBy< K >()    -> pointer by index (0..N-1)
///   - Reset()              -> null all pointers
///   - Swap( other )        -> swap pointers
///   - CopyFrom( other )    -> shallow copy of pointers
///   - MoveFrom( other )    -> shallow move of pointers
///-----------------------------------------------------------------------------
template < typename TI, typename ...Ts >
class MPack;

// final pack
template < typename TI, typename T0 >
class MPack< TI, T0 >
{
public:
	using Type = T0;

	constexpr MPack() noexcept : m_Node( nullptr ) {}

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
	template < TI K, typename T > constexpr T &BaseBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node;
		else
			static_assert( K == 0 , "MPack: index OOB for empty pack" );
	}
	template < TI K, typename T > constexpr const T &BaseBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node;
		else
			static_assert( K == 0, "MPack: index OOB for empty pack" );
	}

	// utilities
	constexpr void Reset() noexcept
	{
		m_Node = nullptr;
	}

protected:
	constexpr void Swap( MPack &other ) noexcept
	{
		Math_Swap( m_Node, other.m_Node );
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
	T0  m_Node;
};

// non-empty pack
template < typename TI, typename T0, typename ...Ts >
class MPack< TI, T0, Ts... >
{
public:
	using Type = T0;
	using Tail_t = MPack< TI, Ts... >;

	constexpr MPack() noexcept : m_Node( nullptr ), m_Tail() {}

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
	template < TI K, typename T > constexpr T &BaseBy() noexcept
	{
		if constexpr ( K == 0 )
			return m_Node;
		else
			return m_Tail.template BaseBy< K - 1 >();
	}
	template < TI K, typename T > constexpr const T &BaseBy() const noexcept
	{
		if constexpr ( K == 0 )
			return m_Node;
		else
			return m_Tail.template BaseBy< K - 1 >();
	}

	// utilities
	constexpr void Reset() noexcept
	{
		m_Node = nullptr;
		m_Tail.Reset();
	}

protected:
	constexpr void Swap( MPack &other ) noexcept
	{
		Math_Swap( m_Node, other.m_Node );
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
	T0     m_Node;
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
class MPointerPack< TI, T0, Ts... > : public MPointerPack< TI, T0 *, Ts... >
{
public:
	using Base_t = MPointerPack< TI, T0 *, Ts... >;
	using Base_t::Base_t;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_PACK_HPP_ )
