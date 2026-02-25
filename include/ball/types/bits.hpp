#ifndef _INCLUDE_BALL_TYPES_BITS_HPP_
#	define _INCLUDE_BALL_TYPES_BITS_HPP_

#	pragma once

#	include "c/bits.h"
#	include "base/arch.h"
#	include "base/fixed.h"
#	include "meta/fixed.hpp"

/// @brief Computes the bit width helper used for power-of-two construction.
/// @tparam I Unsigned integral type described by MFixed<I>.
/// @param x Input value.
/// @return For x == 0, returns 1. For supported widths (<= 64 bits), returns
///         msb_index(x - 1) + 1. Returns MFixed<I>::INVALID for unsupported
///         widths on this implementation.
/// @note For the intrinsic path, non-zero inputs are expected to satisfy x > 1.
template < typename I >
constexpr I BitWidth( I x )
{
	using Fixed_t = MFixed< I >;
	constexpr I NUM_BITS = Fixed_t::BITS;
	constexpr I INVALID = Fixed_t::INVALID;

#	if defined( _MSC_VER )
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

#	elif defined( __GNUC__ ) || defined( __clang__ )
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

#	else
#		error Unsupported platform!
#	endif
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

#endif // !defined( _INCLUDE_BALL_TYPES_BITS_HPP_ )
