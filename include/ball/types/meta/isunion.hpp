#ifndef _INCLUDE_BALL_TYPES_META_ISUNION_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISUNION_HPP_

#	pragma once

// Determine whether T is a union type.
template < typename T > struct MIsUnion { static constexpr bool VALUE = __is_union( T ); };

template < typename T > inline constexpr bool IS_UNION = MIsUnion< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISUNION_HPP_ )
