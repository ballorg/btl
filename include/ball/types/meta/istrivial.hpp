#ifndef _INCLUDE_BALL_TYPES_META_ISTRIVIAL_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISTRIVIAL_HPP_

#	pragma once

// Determine whether T is a trivial type (trivially default-constructible and copyable).
template < typename T > struct MIsTrivial { static constexpr bool VALUE = __is_trivial( T ); };

template < typename T > static constexpr bool IS_TRIVIAL = MIsTrivial< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISTRIVIAL_HPP_ )
