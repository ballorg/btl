#ifndef _INCLUDE_BALL_TYPES_META_GET_HPP_
#	define _INCLUDE_BALL_TYPES_META_GET_HPP_

#	pragma once

#	include "xvalue.hpp"

template < auto K, typename TPACK >
constexpr decltype( auto ) Get( TPACK &pack ) noexcept
{
	return pack.template BaseBy< K >();
}

template < auto K, typename TPACK >
constexpr decltype( auto ) Get( const TPACK &pack ) noexcept
{
	return pack.template BaseBy< K >();
}

template < auto K, typename TPACK >
constexpr decltype( auto ) Get( TPACK &&pack ) noexcept
{
	return Move( pack.template BaseBy< K >() );
}

template < auto K, typename TPACK >
constexpr decltype( auto ) Get( const TPACK &&pack ) noexcept
{
	return Move( pack.template BaseBy< K >() );
}

template < typename T, typename TPACK >
constexpr decltype( auto ) Get( TPACK &pack ) noexcept
{
	return pack.template BaseBy< T >();
}

template < typename T, typename TPACK >
constexpr decltype( auto ) Get( const TPACK &pack ) noexcept
{
	return pack.template BaseBy< T >();
}

template < typename T, typename TPACK >
constexpr decltype( auto ) Get( TPACK &&pack ) noexcept
{
	return Move( pack.template BaseBy< T >() );
}

template < typename T, typename TPACK >
constexpr decltype( auto ) Get( const TPACK &&pack ) noexcept
{
	return Move( pack.template BaseBy< T >() );
}

template < typename T, typename TPACK, typename I >
constexpr decltype( auto ) Get( TPACK &pack, I i ) noexcept
{
	return pack.template At< T >( i );
}

template < typename T, typename TPACK, typename I >
constexpr decltype( auto ) Get( const TPACK &pack, I i ) noexcept
{
	return pack.template At< T >( i );
}

#endif // !defined( _INCLUDE_BALL_TYPES_META_GET_HPP_ )
