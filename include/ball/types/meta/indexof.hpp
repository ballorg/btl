#ifndef _INCLUDE_BALL_TYPES_META_INDEXOF_HPP_
#	define _INCLUDE_BALL_TYPES_META_INDEXOF_HPP_

#	include "issame.hpp"
#	include "number.hpp"

template < typename TI, typename T, typename T0, typename ...Ts >
struct MIndexOf
{
	static constexpr TI VALUE = IS_SAME< T, T0 > ? TI( 0 ) : ( TI( 1 ) + MIndexOf< T, Ts... >::VALUE );
};

template < typename TI, typename T, typename T0 >
struct MIndexOf< TI, T, T0 >
{
	using Number_t = MNumber< TI >;

	static constexpr TI INVALID_TYPE_INDEX = Number_t::INVALID;
	static constexpr TI VALUE = IS_SAME< T, T0 > ? TI( 0 ) : INVALID_TYPE_INDEX;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_INDEXOF_HPP_ )
