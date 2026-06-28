#ifndef _INCLUDE_BALL_TYPES_META_TUPLE_HPP_
#	define _INCLUDE_BALL_TYPES_META_TUPLE_HPP_

#	pragma once

/// Minimal typelist used to store ordered meta elements.
template < typename... Ts >
struct MTuple;

template <>
struct MTuple<>
{
	static constexpr decltype( sizeof( 0 ) ) COUNT = 0;
};

template < typename H, typename... Ts >
struct MTuple< H, Ts... >
{
	using Head_t = H;
	using Tail_t = MTuple< Ts... >;

	static constexpr decltype( sizeof...( Ts ) ) COUNT = 1 + sizeof...( Ts );
};

/// Concatenates two typelists into a single typelist.
template < typename L, typename R >
struct MTupleCat;

template < typename... Ls, typename... Rs >
struct MTupleCat< MTuple< Ls... >, MTuple< Rs... > >
{
	using Type = MTuple< Ls..., Rs... >;
};

template < typename L, typename R >
using TupleCat_t = typename MTupleCat< L, R >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_META_TUPLE_HPP_ )
