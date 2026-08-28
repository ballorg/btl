#ifndef _INCLUDE_BALL_TYPES_META_GET_HPP_
#	define _INCLUDE_BALL_TYPES_META_GET_HPP_

#	pragma once

#	include "xvalue.hpp"

template < class V, typename T >
inline constexpr bool IS_PACKED_STORAGE_BY = V::template TYPE_HAS_PACKED_BITS< T >;

template < class V, typename T >
inline constexpr size_t STORAGE_ALIGNMENT_BY = V::template STORAGE_ALIGNMENT< T >;

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
	return pack.template Get< T >( i );
}

template < typename T, typename TPACK, typename I >
constexpr decltype( auto ) Get( const TPACK &pack, I i ) noexcept
{
	return pack.template Get< T >( i );
}

template < typename T, typename ITERATOR, class V >
constexpr ITERATOR GetBegin( V &view ) noexcept
{
	if constexpr ( IS_PACKED_STORAGE_BY< V, T > )
		return ITERATOR( V::FIRST_INDEX, view.template Packed_BaseBy< T >() );
	else
		return Get< T >( view );
}

template < typename T, typename ITERATOR, class V >
constexpr ITERATOR GetEnd( V &view ) noexcept
{
	if constexpr ( IS_PACKED_STORAGE_BY< V, T > )
		return ITERATOR( view.Count(), view.template Packed_BaseBy< T >() );
	else
		return Get< T >( view ) + view.Count();
}

#	include "get/vector.hpp"

#endif // !defined( _INCLUDE_BALL_TYPES_META_GET_HPP_ )
