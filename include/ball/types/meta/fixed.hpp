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
	using Raw_t = RemoveCV_t< Type >;
	using Signed_t = MSigned< Raw_t >::Type;
	using Unsigned_t = MUnsigned< Raw_t >::Type;

	static constexpr Type SIZE = sizeof( Raw_t );
	static constexpr bool IS_BOOL = IS_SAME< Raw_t, bool >;
	static constexpr Type BYTES = SIZE;
	static constexpr bits_t BITS = IS_BOOL ? 1ull : BYTES * 8ull;
	static constexpr bool IS_UNSIGNED = IS_SAME< Raw_t, Unsigned_t >;
	static constexpr bool IS_SIGNED = !IS_UNSIGNED;

	static constexpr Unsigned_t MIN_UNSIGNED = 0;
	static constexpr Unsigned_t MAX_UNSIGNED = static_cast< Unsigned_t >( ~0ull );
	static constexpr Type MIN_SIGNED = static_cast< Raw_t >( 1ull ) << ( BITS - 1 );
	static constexpr Type ALL_BITS = static_cast< Raw_t >( ~0ull );
	static constexpr Type INVALID = ALL_BITS;
	static constexpr Type MAX_SIGNED = static_cast< Raw_t >( ~MIN_SIGNED );
	static constexpr Unsigned_t MIN = IS_SIGNED ? MIN_SIGNED : MIN_UNSIGNED;
	static constexpr Unsigned_t MAX = IS_SIGNED ? MAX_SIGNED : MAX_UNSIGNED;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_FIXED_HPP_ )
