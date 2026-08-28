#ifndef _INCLUDE_BALL_TYPES_META_ISDEFAULTCONSTRUCTIBLE_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISDEFAULTCONSTRUCTIBLE_HPP_

#	pragma once

// Determine whether T can be constructed with no arguments.
template < typename T > struct MIsDefaultConstructible { static constexpr bool VALUE = __is_constructible( T ); };

template < typename T > inline constexpr bool IS_DEFAULT_CONSTRUCTIBLE = MIsDefaultConstructible< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISDEFAULTCONSTRUCTIBLE_HPP_ )
