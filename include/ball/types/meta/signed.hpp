#ifndef _INCLUDE_BALL_TYPES_META_SIGNED_HPP_
#	define _INCLUDE_BALL_TYPES_META_SIGNED_HPP_

#	pragma once

#	include "issame.hpp"
#	include "select.hpp"
#	include "removecv.hpp"

// Specializations by size.
#	define BALL_SIGNED_DECLARE( size, ... ) \
	template <> struct MSignedSelect< size > \
	{ \
		template < typename T > using Apply_t = __VA_ARGS__; \
	}

// Choose make_signed strategy by type size
template < unsigned long int > struct MSignedSelect;
BALL_SIGNED_DECLARE( 1, signed char );
BALL_SIGNED_DECLARE( 2, signed short );
BALL_SIGNED_DECLARE( 4, typename MSelect< IS_SAME< T, long int > || IS_SAME< T, unsigned long int > >::template Apply_t< signed long int, signed int > );
BALL_SIGNED_DECLARE( 8, typename MSelect< IS_SAME< T, long int > || IS_SAME< T, unsigned long int > >::template Apply_t< signed long int, signed long long int > );

// Signed partner to cv-unqualified T
template < class T > using SignedSelect_t = typename MSignedSelect< sizeof( T ) >::template Apply_t<T>;

template < typename T >
struct MSigned
{
	using Type = typename MRemoveCV< T >::template Apply_t< SignedSelect_t >;
};
template < typename T >
using Signed_t = typename MSigned< T >::Type;

#	undef BALL_SIGNED_DECLARE

#endif // !defined( _INCLUDE_BALL_TYPES_META_SIGNED_HPP_ )
