#ifndef _INCLUDE_BALL_TYPES_META_INDEXOF_HPP_
#	define _INCLUDE_BALL_TYPES_META_INDEXOF_HPP_

#	pragma once

#	include "issame.hpp"
#	include "fixed.hpp"

template < typename TI, typename T, typename T0, typename ...Ts >
struct MIndexOf
{
	static constexpr TI VALUE = IS_SAME< T, T0 > ? TI( 0 ) : ( TI( 1 ) + MIndexOf< TI, T, Ts... >::VALUE );
};

template < typename TI, typename T, typename T0 >
struct MIndexOf< TI, T, T0 >
{
	using Fixed_t = MFixed< TI >;

	static constexpr TI INVALID_TYPE_INDEX = Fixed_t::INVALID;
	static constexpr TI VALUE = IS_SAME< T, T0 > ? TI( 0 ) : INVALID_TYPE_INDEX;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_INDEXOF_HPP_ )
