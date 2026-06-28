#ifndef _INCLUDE_BALL_TYPES_META_INDEXTYPE_HPP_
#	define _INCLUDE_BALL_TYPES_META_INDEXTYPE_HPP_

#	pragma once

template < typename TI, TI K, typename... Ts >
struct MIndexType;

template < bool IS_ZERO, typename TI, TI K, typename... Ts >
struct MIndexTypeBy;

template < typename TI, TI K, typename T0, typename... Ts >
struct MIndexTypeBy< true, TI, K, T0, Ts... >
{
	using Type = T0;
};

template < typename TI, TI K, typename T0, typename... Ts >
struct MIndexTypeBy< false, TI, K, T0, Ts... >
{
	using Type = typename MIndexType< TI, K - 1, Ts... >::Type;
};

template < typename TI, TI K, typename... Ts >
struct MIndexType : MIndexTypeBy< K == 0, TI, K, Ts... >
{
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_INDEXTYPE_HPP_ )
