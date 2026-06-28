#ifndef _INCLUDE_BALL_TYPES_META_INDEXOF_HPP_
#	define _INCLUDE_BALL_TYPES_META_INDEXOF_HPP_

#	pragma once

#	include "issame.hpp"

template < typename TI, typename T, typename... Ts > struct MIndexOf;

template < typename TI, typename T, typename T0, typename... Ts >
struct MIndexOf< TI, T, T0, Ts... >
{
	static constexpr int VALUE = IS_SAME< T, T0 > ? 0 : MIndexOf< TI, T, Ts... >::VALUE;
};

template < typename TI, typename T, typename T0 >
struct MIndexOf< TI, T, T0 >
{
	static constexpr int VALUE = IS_SAME< T, T0 > ? 0 : -1;
};

template < typename TI, typename T >
struct MIndexOf< TI, T >
{
	static constexpr int VALUE = -1;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_INDEXOF_HPP_ )
