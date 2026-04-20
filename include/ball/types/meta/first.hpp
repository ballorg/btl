#ifndef _INCLUDE_BALL_TYPES_META_FIRST_HPP_
#	define _INCLUDE_BALL_TYPES_META_FIRST_HPP_

#	pragma once

template < typename T0, typename... Ts >
struct MFirst
{
	using Type = T0;
};

template < typename... Ts >
using First_t = typename MFirst< Ts... >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_META_FIRST_HPP_ )
