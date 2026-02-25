#ifndef _INCLUDE_BALL_TYPES_MATH_HPP_
#	define _INCLUDE_BALL_TYPES_MATH_HPP_

#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/math.h"
#	include "elements.hpp"
#	include "bits.hpp"
#	include "xvalue.hpp"

// Swap two of anything.
template < class T >
inline void Math_Swap( T &x, T &y ) noexcept
{
	T temp = x;

	x = y;
	y = Move( temp );
}

template < class T, size_t N >
inline void Math_SwapN( T ( &x )[ N ], T ( &y )[ N ] ) noexcept
{
	T temp[ N ] = x;

	x = y;
	y = Move( temp );
}

/// @brief Computes power of 10 at compile/runtime without floating point.
/// @tparam P The exponent.
/// @return 10^P as unsigned long long.
template < size_t P >
inline ullong_t Math_Pow10() noexcept
{
	ullong_t v = 1ull;

	for ( size_t n = 0; n < P; ++n )
		v *= 10ull;

	return v;
}

/// @brief Checks whether @p x is an exact power of two.
///
/// @param x Value to test.
///
/// @return Non-zero if x is a power of two, 0 otherwise.
///
/// @details Relies on the property: (x & (x - 1)) == 0 for powers-of-two.
inline int Math_IsPow2( size_t x )
{
	return x && ( ( x & ( x - 1 ) ) == 0 );
}

/// @brief Rounds @p v up to the nearest multiple of @p g.
///
/// @param v Value to round.
/// @param g Alignment/granularity (must be > 0).
///
/// @return v aligned upward to the next multiple of g.
///
/// @details Classic bitmask rounding: add (g - 1), then clear lower bits.
inline size_t Math_RoundUp( size_t v, size_t g )
{
	return ( v + ( g - 1 ) ) & ~( g - 1 );
}


/// @brief Floor(log2(x)) for unsigned integers. For x==0 returns 0.
template < typename I >
constexpr I Math_Log2_Floor( I x ) noexcept
{
	I w = 0;

	while ( x >>= 1 )
		++w;

	return w;
}

template < typename I, uint8_t NS >
constexpr I Math_Log2Pow2() noexcept
{
	static_assert( NS >= 2, "Base must be >= 2" );
	static_assert( ( NS & ( NS - 1 ) ) == 0, "Base must be power-of-two" );

	uint_t n = static_cast< uint_t >( NS );
	I k = I( 0 );

	while ( n > 1u )
	{
		n >>= 1u;
		++k;
	}

	return k;
}

/// @brief Bit width (number of bits needed to represent x). For x==0 returns 1.
template < typename I >
constexpr I Math_BitWidth( I x ) noexcept
{
	return BitWidth< I >( x );
}

/// @brief Floor(log_NS(x)) for compile-time base NS ∈ [2, 36].
/// @details Generic slow path uses integer division; fast-paths for 16/2.
template < typename I, uint8_t NS >
constexpr I Math_Log_Floor( I x ) noexcept
{
	static_assert( NS >= 2 && NS <= 36, "Math_Log_Floor: base must be in [2,36]" );

	if ( x == 0 )
		return 0;

	if constexpr ( NS == 2 )
	{
		return Math_Log2_Floor( x );
	}
	else if constexpr ( NS == 16 )
	{
		const I bw = Math_BitWidth< I >( x );

		return ( bw - 1u ) / 4u;
	}
	else
	{
		I k = 0;

		while ( x >= I( NS ) )
		{
			x /= I( NS );
			++k;
		}

		return k;
	}
}

/// @brief Number of digits in base-NS for value x (x==0 -> 1).
template < typename I, uint8_t NS >
constexpr I Math_Digits( I x ) noexcept
{
	return ( x == 0 ) ? 1u : ( Math_Log_Floor< I, NS >( x ) + 1u );
}

#endif // !defined( _INCLUDE_BALL_TYPES_MATH_HPP_ )
