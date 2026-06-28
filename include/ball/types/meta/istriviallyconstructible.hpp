#ifndef _INCLUDE_BALL_TYPES_META_ISTRIVIALLYCONSTRUCTIBLE_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISTRIVIALLYCONSTRUCTIBLE_HPP_

#	pragma once

// Determine whether T is trivially default-constructible.
template < typename T > struct MIsTriviallyConstructible { static constexpr bool VALUE = __is_trivially_constructible( T ); };

template < typename T > static constexpr bool IS_TRIVIALLY_CONSTRUCTIBLE = MIsTriviallyConstructible< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISTRIVIALLYCONSTRUCTIBLE_HPP_ )
