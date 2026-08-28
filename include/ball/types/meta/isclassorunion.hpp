#ifndef _INCLUDE_BALL_TYPES_META_ISCLASSORUNION_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISCLASSORUNION_HPP_

#	pragma once

#	include "isclass.hpp"
#	include "isunion.hpp"

// Determine whether T is a class or a union type.
template < typename T > struct MIsClassOrUnion { static constexpr bool VALUE = MIsClass< T >::VALUE || MIsUnion< T >::VALUE; };

template < typename T > inline constexpr bool IS_CLASS_OR_UNION = MIsClassOrUnion< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISCLASSORUNION_HPP_ )
