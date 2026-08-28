#ifndef _INCLUDE_BALL_TYPES_META_ISCLASS_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISCLASS_HPP_

#	pragma once

// Determine whether T is a non-union class type.
template < typename T > struct MIsClass { static constexpr bool VALUE = __is_class( T ); };

template < typename T > inline constexpr bool IS_CLASS = MIsClass< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISCLASS_HPP_ )
