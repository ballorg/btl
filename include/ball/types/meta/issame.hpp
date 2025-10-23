#ifndef _INCLUDE_BALL_TYPES_META_ISSAME_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISSAME_HPP_

// Determine whether arguments are the same type.
template < typename, typename > constexpr bool IS_SAME = false;
template < typename T > constexpr bool IS_SAME< T, T > = true;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISSAME_HPP_ )
