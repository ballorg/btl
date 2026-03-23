#ifndef _INCLUDE_BALL_TYPES_PAIR_HPP_
#	define _INCLUDE_BALL_TYPES_PAIR_HPP_

#	pragma once

#	include "meta/xvalue.hpp"

template < typename K, typename V >
class CPair
{
public:
	constexpr CPair( const K &key, const V &value ) noexcept : m_Key( key ), m_Value( value ) {}
	constexpr CPair( const K &key, V &&value ) noexcept : m_Key( key ), m_Value( Move( value ) ) {}
	constexpr CPair( K &&first, const V &second ) noexcept : m_Key( Move( first ) ), m_Value( second ) {}
	constexpr CPair( K &&first, V &&second ) noexcept : m_Key( Move( first ) ), m_Value( Move( second ) ) {}

	constexpr K &Key() noexcept { return m_Key; }
	constexpr V &Value() noexcept { return m_Value; }
	constexpr const K &Key() const noexcept { return m_Key; }
	constexpr const V &Value() const noexcept { return m_Value; }

	constexpr K &First() noexcept { return Key(); }
	constexpr V &Second() noexcept { return Value(); }
	constexpr const K &First() const noexcept { return Key(); }
	constexpr const V &Second() const noexcept { return Value(); }

	bool operator==( const K &other ) const noexcept { return Key() == other; }
	bool operator!=( const K &other ) const noexcept { return Key() != other; }
	bool operator< ( const K &other ) const noexcept { return Key() <  other; }

	bool operator==( const CPair &other ) const noexcept { return Key() == other.Key() && Value() == other.Value(); }
	bool operator!=( const CPair &other ) const noexcept { return Key() != other.Key() && Value() != other.Value(); }
	bool operator< ( const CPair &other ) const noexcept { return Key() <  other.Key() && Value() <  other.Value(); }

private:
	K m_Key;
	V m_Value;
};

template < typename K, typename V > using Pair_t = CPair< K, V >;

#endif // !defined( _INCLUDE_BALL_TYPES_PAIR_HPP_ )
