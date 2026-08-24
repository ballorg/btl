#ifndef _INCLUDE_BALL_TYPES_META_TYPECOUNT_HPP_
#	define _INCLUDE_BALL_TYPES_META_TYPECOUNT_HPP_

#	pragma once

template < typename... Ts >
static constexpr decltype( sizeof...( Ts ) ) TYPE_COUNT = sizeof...( Ts );

#endif // !defined( _INCLUDE_BALL_TYPES_META_TYPECOUNT_HPP_ )
