#ifndef _INCLUDE_BALL_TYPES_META_ISCOPYCONSTRUCTIBLE_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISCOPYCONSTRUCTIBLE_HPP_

#	pragma once

// Determine whether T can be constructed from a const lvalue of T.
template < typename T > struct MIsCopyConstructible { static constexpr bool VALUE = __is_constructible( T, const T & ); };

template < typename T > inline constexpr bool IS_COPY_CONSTRUCTIBLE = MIsCopyConstructible< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISCOPYCONSTRUCTIBLE_HPP_ )
