#ifndef _INCLUDE_BALL_TYPES_PAIR_HPP_
#	define _INCLUDE_BALL_TYPES_PAIR_HPP_

#	pragma once

#	include "meta/xvalue.hpp"

template < typename K, typename V >
struct Pair_t
{
	constexpr Pair_t( const K &key = K(), const V &value = V() ) noexcept : m_Key( key ), m_Value( value ) {}
	constexpr Pair_t( const K &key, V &&value ) noexcept : m_Key( key ), m_Value( Move( value ) ) {}
	constexpr Pair_t( K &&first, const V &second ) noexcept : m_Key( Move( first ) ), m_Value( second ) {}
	constexpr Pair_t( K &&first, V &&second ) noexcept : m_Key( Move( first ) ), m_Value( Move( second ) ) {}

	constexpr K &Key() noexcept { return m_Key; }
	constexpr V &Value() noexcept { return m_Value; }
	constexpr const K &Key() const noexcept { return m_Key; }
	constexpr const V &Value() const noexcept { return m_Value; }

	constexpr K &First() noexcept { return m_Key; }
	constexpr V &Second() noexcept { return m_Value; }
	constexpr const K &First() const noexcept { return m_Key; }
	constexpr const V &Second() const noexcept { return m_Value; }

	bool operator==( const K &other ) const noexcept { return m_Key == other; }
	bool operator!=( const K &other ) const noexcept { return m_Key != other; }
	bool operator< ( const K &other ) const noexcept { return m_Key <  other; }

	bool operator==( const Pair_t &other ) const noexcept { return m_Key == other.m_Key && m_Value == other.m_Value; }
	bool operator!=( const Pair_t &other ) const noexcept { return m_Key != other.m_Key && m_Value != other.m_Value; }
	bool operator< ( const Pair_t &other ) const noexcept { return m_Key <  other.m_Key && m_Value <  other.m_Value; }

	K m_Key;
	V m_Value;
};

#endif // !defined( _INCLUDE_BALL_TYPES_PAIR_HPP_ )
