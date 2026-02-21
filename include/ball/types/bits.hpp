#ifndef _INCLUDE_BALL_TYPES_BITS_HPP_
#	define _INCLUDE_BALL_TYPES_BITS_HPP_

#	pragma once

#	include "c/bits.h"
#	include "base/arch.h"
#	include "base/fixed.h"
#	include "meta/fixed.hpp"

/// @brief Returns the smallest power of two greater than or equal to @p v,
///        saturating to the highest power-of-two representable in T.
/// @tparam T Unsigned integral type (e.g., uint32_t, uint64_t).
///
/// @details
/// - For v <= 1, returns 1.
/// - For v already a power of two, returns v.
/// - If the exact ceiling power-of-two does not fit in T, returns the
///   highest power-of-two of T (i.e., 1 << (bit_width(T) - 1 ) ).
/// - Uses compiler intrinsics where available; otherwise falls back to a
///   portable bit-spreading implementation.
/// - Precondition: T must be an uint_t integral type.
///
/// @note This function implements semantics similar to std::bit_ceil 
///       but saturates instead of overflowing/returning 0 on overflow.
///
/// @return ceil_pow2(v) clamped to the type’s top power-of-two.
/// @brief Floor(log2(x)) for unsigned integers. For x==0 returns 0.
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

template < typename I >
consteval I BitCeil_Const( I x )
{
	return BitCeil_Unified( x );
}

template < typename I >
constexpr I BitCeil( I x )
{
	using Fixed_t = MFixed< I >;
	constexpr I NUM_BITS = Fixed_t::BITS;
	constexpr I INVALID = Fixed_t::INVALID;

	if ( x <= 1 )
	{
		return 1;
	}
#	if defined( _MSC_VER )
	// MSVC (_BitScanReverse/_BitScanReverse64), supports to 64 bits
	else if constexpr ( NUM_BITS <= 32 )
	{
		ulong_t i;
		uint_t nMask = static_cast< uint_t >( x - 1 );

		_BitScanReverse( &i, nMask );

		uint_t s = static_cast< uint_t >( i ) + 1;

		return ( s >= NUM_BITS ) ? INVALID : ( I( 1 ) << s );
	}
	else if constexpr ( NUM_BITS <= 64 )
	{
		ulong_t i;
		ullong_t nMask = static_cast< ullong_t >( x - 1 );

		_BitScanReverse64( &i, nMask );
		uint_t s = static_cast< uint_t >( i ) + 1;

		return ( s >= NUM_BITS ) ? INVALID : ( I( 1 ) << s );
	}
	else
		return INVALID;

#	elif defined( __GNUC__ ) || defined( __clang__ )
	// GCC/Clang: __builtin_clz*/__builtin_clzll, supports to 64 bits
	else if constexpr ( NUM_BITS <= 32 )
	{
		// clz(0) — UB, но x != 0 т.к. v > 1
		uint_t s = 32u - static_cast< uint_t >( __builtin_clz( static_cast< uint_t >( x - 1 ) ) );

		return ( s >= NUM_BITS ) ? INVALID : ( I( 1 ) << s );
	}
	else if constexpr ( NUM_BITS <= 64 )
	{
		uint_t s = 64u - static_cast< uint_t >( __builtin_clzll( static_cast< ullong_t >( x - 1 ) ) );

		return ( s >= NUM_BITS ) ? INVALID : ( I( 1 ) << s );
	}
	else
		return INVALID;

#	else
#		error Unsupported platform!
#	endif
}

/// @brief Floor(log_NS(x)) for compile-time base NS ∈ [2, 36].
/// @details Generic slow path uses integer division; fast-paths for 16/2.
template < typename I = uint_t, uint8_t NS, typename U >
constexpr I BitCeil( U x ) noexcept
{
	static_assert( NS >= 2 && NS <= 36, "BitCeil: base must be in [2,36]" );

	if ( x == 0 )
		return 0;

	if constexpr ( NS == 2 )
	{
		return BitCeil_Floor( x );
	}
	else if constexpr ( NS == 16 )
	{
		const I bw = BitCeil< I >( x );

		return ( bw - 1u ) / 4u;
	}
	else
	{
		I k = 0;

		while ( x >= U( NS ) )
		{
			x /= U( NS );
			++k;
		}

		return k;
	}
}

#endif // !defined( _INCLUDE_BALL_TYPES_BITS_HPP_ )
