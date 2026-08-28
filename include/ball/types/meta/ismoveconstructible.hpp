#ifndef _INCLUDE_BALL_TYPES_META_ISMOVECONSTRUCTIBLE_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISMOVECONSTRUCTIBLE_HPP_

#	pragma once

// Determine whether T can be constructed from an rvalue of T.
template < typename T > struct MIsMoveConstructible { static constexpr bool VALUE = __is_constructible( T, T && ); };

template < typename T > inline constexpr bool IS_MOVE_CONSTRUCTIBLE = MIsMoveConstructible< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISMOVECONSTRUCTIBLE_HPP_ )
