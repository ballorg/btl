#ifndef _INCLUDE_BALL_TYPES_META_NUMBER_HPP_
#	define _INCLUDE_BALL_TYPES_META_NUMBER_HPP_

#	include "issame.hpp"
#	include "select.hpp"
#	include "removecv.hpp"

// Specializations by size.
#	define BALL_MUNSIGNEDSELECT_DECLARE( size, ... ) \
	template <> struct MUnsignedSelect< size > \
	{ \
		template < typename T > using Apply_t = __VA_ARGS__; \
	}

// Choose make_unsigned strategy by type size
template < unsigned long int > struct MUnsignedSelect;
BALL_MUNSIGNEDSELECT_DECLARE( 1, unsigned char );
BALL_MUNSIGNEDSELECT_DECLARE( 2, unsigned short );
BALL_MUNSIGNEDSELECT_DECLARE( 4, typename MSelect< IS_SAME< T, long int > || IS_SAME< T, unsigned long int > >::template Apply_t< unsigned long int, unsigned int > );
BALL_MUNSIGNEDSELECT_DECLARE( 8, typename MSelect< IS_SAME< T, long int > || IS_SAME< T, unsigned long int > >::template Apply_t< unsigned long int, unsigned long long int > );

// unsigned partner to cv-unqualified _Ty
template < class T > using UnsignedSelect_t = typename MUnsignedSelect< sizeof( T ) >::template Apply_t<T>;

// Unsigned number.
template < typename T >
struct MUnsigned
{
	using Type = typename MRemoveCV< T >::template Apply_t< UnsignedSelect_t >;
};
template < typename T >
using Unsigned_t = typename MUnsigned< T >::Type;

// Any integer number.
template < typename T >
struct MNumber
{
	using U = Unsigned_t< T >;

	static constexpr T SIZE = sizeof( T );
	static constexpr T BYTES = SIZE;
	static constexpr T BITS = BYTES * 8;

	static constexpr bool IS_UNSIGNED = IS_SAME< T, U >;
	static constexpr bool IS_SIGNED = !IS_UNSIGNED;

	static constexpr U MIN_UNSIGNED = 0;
	static constexpr U MAX_UNSIGNED = static_cast< U >( ~0ull );
	static constexpr T MIN_SIGNED = static_cast< T >( 1u ) << ( BITS - 1 );
	static constexpr T ALL_BITS = static_cast< T >( ~0ull );
	static constexpr T INVALID = ALL_BITS;
	static constexpr T MAX_SIGNED = static_cast< T >( ~MIN_SIGNED );
	static constexpr U MIN = IS_SIGNED ? MIN_SIGNED : MIN_UNSIGNED;
	static constexpr U MAX = IS_SIGNED ? MAX_SIGNED : MAX_UNSIGNED;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_NUMBER_HPP_ )
