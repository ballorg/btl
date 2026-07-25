#ifndef _INCLUDE_BALL_TYPES_BITS_HPP_
#	define _INCLUDE_BALL_TYPES_BITS_HPP_

#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/assert.h"
#	include "c/bits.h"
#	include "meta/fixed.hpp"

/// @brief Computes the bit width helper used for power-of-two construction.
/// @tparam I Unsigned integral type described by MFixed<I>.
/// @param x Input value.
/// @return For supported widths (<= 64 bits), returns msb_index(x - 1) + 1.
///         Returns MFixed<I>::INVALID for unsupported widths on this implementation.
/// 
/// @note Precondition: x > 1. The intrinsic path scans the mask `x - 1`; a zero
///       mask (x == 0 or x == 1) leaves the scan's index output undefined, so
///       callers must guard those cases (see BitCeil). Asserted below.
template < typename I >
constexpr I BitWidth( I x )
{
	using Fixed_t = MFixed< I >;
	constexpr I NUM_BITS = Fixed_t::BITS;
	constexpr I INVALID = Fixed_t::INVALID;

#	if defined( BALL_MSVC )
	if constexpr ( NUM_BITS <= 32 )
	{
		ulong_t i; // Bit index returned by the scan intrinsic.

		_BitScanReverse( &i, static_cast< uint_t >( x - !!x ) );

		return i + 1;
	}
	else if constexpr ( NUM_BITS <= 64 )
	{
		ulong_t i; // Bit index returned by the scan intrinsic.

		_BitScanReverse64( &i, static_cast< ullong_t >( x - !!x ) );

		return i + 1;
	}
	else
		return INVALID;

#	elif defined( BALL_GNUC )
	// GCC/Clang: use __builtin_clz*/__builtin_clzll for up to 64-bit inputs.
	if constexpr ( NUM_BITS <= 32 )
	{
		uint_t i = 32u - static_cast< uint_t >( __builtin_clz( static_cast< uint_t >( x - !!x ) ) );

		return i + 1;
	}
	else if constexpr ( NUM_BITS <= 64 )
	{
		uint_t i = 64u - static_cast< uint_t >( __builtin_clzll( static_cast< ullong_t >( x - !!x ) ) );

		return i + 1;
	}
	else
		return INVALID;

#	else // !defined( BALL_MSVC ) && !defined( BALL_GNUC )
#		error Unsupported platform!
#	endif // defined( BALL_MSVC ) || defined( BALL_GNUC )
}

/// @brief Returns the smallest power of two greater than or equal to @p x.
/// @tparam I Unsigned integral type (e.g., uint32_t, uint64_t).
///
/// @details
/// - For x <= 1, returns 1.
/// - If x is already a power of two, returns x.
/// - If the exact ceiling power of two overflows I, returns the largest
///   power of two representable by I (i.e., 1 << (bit_width(I) - 1)).
/// - Uses compiler intrinsics where available; otherwise falls back to a
///   portable bit-spreading implementation.
/// - Precondition: I must be an unsigned integral type.
///
/// @note Semantics are similar to std::bit_ceil, but this implementation
///       saturates instead of overflowing (or returning 0 on overflow).
///
/// @return ceil_pow2(x), clamped to the type's highest power of two.
template < typename I >
static constexpr I BitCeil_Unified( I x ) noexcept
{
	constexpr I NUM_BITS = MFixed< I >::BITS;

	if ( x <= 1 )
		return I( 1 );

	--x;
	x |= ( x >> 1 );
	x |= ( x >> 2 );
	x |= ( x >> 4 );

	if constexpr ( NUM_BITS >  8 ) x |= ( x >> 8  );
	if constexpr ( NUM_BITS > 16 ) x |= ( x >> 16 );
	if constexpr ( NUM_BITS > 32 ) x |= ( x >> 32 );

	++x;

	return x ? x : I( 1 ) << ( NUM_BITS - 1 );
}

/// @brief Compile-time wrapper around BitCeil_Unified.
/// @tparam I Unsigned integral type.
/// @param x Input value.
/// @return Same value as BitCeil_Unified(x), evaluated at compile time.
template < typename I >
consteval I BitCeil_Const( I x )
{
	return BitCeil_Unified( x );
}

/// @brief Computes a power of two from the width returned by BitWidth.
/// @tparam I Unsigned integral type described by MFixed<I>.
/// @param x Input value.
/// @return I(1) << BitWidth(x), or MFixed<I>::INVALID when width computation
///         is unsupported for the target type.
template < typename I >
constexpr I BitCeil( I x )
{
	constexpr I INVALID = MFixed< I >::INVALID;

	I iWidth = BitWidth( x );

	if ( iWidth == INVALID )
		return INVALID;

	return I( 1 ) << iWidth;
}

/// @brief Population count: the number of set (1) bits in @p x.
/// @tparam U Unsigned integral type described by MFixed<U> (width <= 64). A
///         population count is defined over the raw bit pattern, so the word must
///         be unsigned: a signed type would sign-extend in the intrinsic casts.
/// @param x Input value.
/// @return The count of 1-bits in @p x, via the platform popcount intrinsic
///         (`__popcnt`/`__popcnt64` on MSVC, `__builtin_popcount*` on GCC/Clang).
template < typename U >
constexpr U PopCount( U x )
{
	using Fixed_t = MFixed< U >;

	BALL_STATIC_ASSERT( Fixed_t::IS_UNSIGNED, "PopCount requires an unsigned integral type (signed inputs would sign-extend)" );

	constexpr U NUM_BITS = Fixed_t::BITS;

#	if defined( BALL_MSVC )
	if constexpr ( NUM_BITS <= 32 )
		return static_cast< U >( __popcnt( static_cast< uint_t >( x ) ) );
	else
		return static_cast< U >( __popcnt64( static_cast< ullong_t >( x ) ) );

#	elif defined( BALL_GNUC )
	if constexpr ( NUM_BITS <= 32 )
		return static_cast< U >( __builtin_popcount( static_cast< uint_t >( x ) ) );
	else
		return static_cast< U >( __builtin_popcountll( static_cast< ullong_t >( x ) ) );

#	else // !defined( BALL_MSVC ) && !defined( BALL_GNUC )
#		error Unsupported platform!
#	endif // defined( BALL_MSVC ) || defined( BALL_GNUC )
}

#endif // !defined( _INCLUDE_BALL_TYPES_BITS_HPP_ )
