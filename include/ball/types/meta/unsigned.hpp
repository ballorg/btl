#ifndef _INCLUDE_BALL_TYPES_META_UNSIGNED_HPP_
#	define _INCLUDE_BALL_TYPES_META_UNSIGNED_HPP_

#	pragma once

#	include "issame.hpp"
#	include "select.hpp"
#	include "removecv.hpp"

// Specializations by size.
#	define BALL_UNSIGNED_SELECTOR( size, ... ) \
	template <> struct MUnsignedSelect< size > \
	{ \
		template < typename T > using Apply_t = __VA_ARGS__; \
	}

// Choose make_unsigned strategy by type size
template < unsigned long int > struct MUnsignedSelect;
BALL_UNSIGNED_SELECTOR( 1, unsigned char );
BALL_UNSIGNED_SELECTOR( 2, unsigned short );
BALL_UNSIGNED_SELECTOR( 4, typename MSelect< IS_SAME< T, long int > || IS_SAME< T, unsigned long int > >::template Apply_t< unsigned long int, unsigned int > );
BALL_UNSIGNED_SELECTOR( 8, typename MSelect< IS_SAME< T, long int > || IS_SAME< T, unsigned long int > >::template Apply_t< unsigned long int, unsigned long long int > );

// Unsigned partner to cv-unqualified T
template < class T > using UnsignedSelect_t = typename MUnsignedSelect< sizeof( T ) >::template Apply_t<T>;

template < typename T >
struct MUnsigned
{
	using Type = typename MRemoveCV< T >::template Apply_t< UnsignedSelect_t >;
};
template < typename T >
using Unsigned_t = typename MUnsigned< T >::Type;

#	undef BALL_UNSIGNED_SELECTOR

#endif // !defined( _INCLUDE_BALL_TYPES_META_UNSIGNED_HPP_ )
