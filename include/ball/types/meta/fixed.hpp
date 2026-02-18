#ifndef _INCLUDE_BALL_TYPES_META_FIXED_HPP_
#	define _INCLUDE_BALL_TYPES_META_FIXED_HPP_

#	pragma once

#	include "fixed/signed.hpp"
#	include "fixed/uncertain.hpp"
#	include "fixed/unsigned.hpp"
#	include "issame.hpp"
#	include "removecv.hpp"
#	include "select.hpp"
#	include "signed.hpp"
#	include "unsigned.hpp"

// Any fixed integer.
template < typename T >
struct MFixed
{
	using Type = T;
	using Singed_t = MSigned< Type >::Type;
	using Unsigned_t = MUnsigned< Type >::Type;

	static constexpr Type SIZE = sizeof( Type );
	static constexpr bool IS_BOOL = IS_SAME< Type, bool >;
	static constexpr Type BYTES = SIZE;
	static constexpr bits_t BITS = IS_BOOL ? bits_t( 1ull ) : bits_t( BYTES * 8ull );
	static constexpr bool IS_UNSIGNED = IS_SAME< Type, Unsigned_t >;
	static constexpr bool IS_SIGNED = !IS_UNSIGNED;

	static constexpr Unsigned_t MIN_UNSIGNED = 0;
	static constexpr Unsigned_t MAX_UNSIGNED = static_cast< Unsigned_t >( ~0ull );
	static constexpr Type MIN_SIGNED = static_cast< Type >( 1ull ) << ( BITS - 1 );
	static constexpr Type ALL_BITS = static_cast< Type >( ~0ull );
	static constexpr Type INVALID = ALL_BITS;
	static constexpr Type MAX_SIGNED = static_cast< Type >( ~MIN_SIGNED );
	static constexpr Unsigned_t MIN = IS_SIGNED ? MIN_SIGNED : MIN_UNSIGNED;
	static constexpr Unsigned_t MAX = IS_SIGNED ? MAX_SIGNED : MAX_UNSIGNED;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_FIXED_HPP_ )
