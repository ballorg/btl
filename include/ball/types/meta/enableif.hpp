#ifndef _INCLUDE_BALL_TYPES_META_ENABLEIF_HPP_
#	define _INCLUDE_BALL_TYPES_META_ENABLEIF_HPP_

template < bool V, typename T = void >
struct MEnableIf {};

template < typename T >
struct MEnableIf< true, T >
{
	using Type = T;
};

template < bool V, typename T = void >
using EnableIf_t = typename MEnableIf< V, T >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ENABLEIF_HPP_ )
