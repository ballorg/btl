#ifndef _INCLUDE_BALL_TYPES_META_REFLECTVALUE_HPP_
#	define _INCLUDE_BALL_TYPES_META_REFLECTVALUE_HPP_

#	pragma once

template < typename T, typename F > class CReflect;

/// Extracts the payload type from a reflected storage wrapper.
template < typename T >
struct MReflectValue
{
	using Type = T;
};

template < typename T, typename F >
struct MReflectValue< CReflect< T, F > >
{
	using Type = T;
};

template < typename T >
using ReflectValue_t = typename MReflectValue< T >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_META_REFLECTVALUE_HPP_ )
