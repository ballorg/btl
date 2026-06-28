#ifndef _INCLUDE_BALL_TYPES_META_REMOVECV_HPP_
#	define _INCLUDE_BALL_TYPES_META_REMOVECV_HPP_

#	pragma once

template < typename T >
struct MRemoveCV
{
	using Type = T;

	template < template < class > class F > using Apply_t = F< T >;
};

template < typename T >
struct MRemoveCV< const T >
{
	using Type = T;

	template < template < class > class F > using Apply_t = const F< T >;
};

template < typename T >
struct MRemoveCV< volatile T >
{
	using Type = T;

	template < template < class > class F > using Apply_t = volatile F< T >;
};

template < typename T >
struct MRemoveCV< const volatile T >
{
	using Type = T;

	template < template < class > class F > using Apply_t = const volatile F< T >;
};

template < typename T > using RemoveCV_t = typename MRemoveCV< T >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_META_REMOVECV_HPP_ )
