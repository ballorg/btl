#ifndef _INCLUDE_BALL_TYPES_NUMBER_HPP_
#	define _INCLUDE_BALL_TYPES_NUMBER_HPP_

#	include "base/arch.h"
#	include "base/fixed.h"

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

///-----------------------------------------------------------------------------
/// @brief Minimal bit-width (count of significant bits), 0 -> 0.
///-----------------------------------------------------------------------------
template < typename U, typename I = uint_t >
constexpr I Num_BitWidth( U x ) noexcept
{
	I w = 0;

	while ( x )
	{
		++w;
		x >>= 1;
	}

	return w;
}
template < typename U, typename I = uint_t >
consteval I Num_BitWidth_Const( U x ) noexcept
{
	return Num_BitWidth( x );
}

///-----------------------------------------------------------------------------
/// @brief Count of base-NS digits required to represent unsigned @p u.
///        Returns at least 1 (i.e., u==0 => 1).
///        Optimized paths for NS = 2, 4, 8, 16 (powers of two).
///-----------------------------------------------------------------------------
template < typename I = uint_t, uint8_t NS, typename U >
constexpr I Num_DigitsNS( U u ) noexcept
{
	static_assert( NS >= 2, "Base must be >= 2" );

	if ( u == 0 )
		return I( 1 );

	if constexpr ( ( NS & ( NS - 1 ) ) == 0 )
	{
		// Power-of-two base: digits = ceil(bit_width / log2(NS)).
		constexpr I k = Num_BitWidth_Const( NS );
		const I bw = Num_BitWidth( u );

		return I( ( bw + k - 1 ) / k );
	}
	else
	{
		// Generic base: repeated division.
		constexpr U BASE = U( NS );

		I nDigits = 0;

		do { ++nDigits; u /= BASE; } while ( u );

		return nDigits;
	}
}

//-----------------------------------------------------------------------------
// Digits -> chars
//-----------------------------------------------------------------------------

template < typename T = uchar_t, typename I = uint_t, typename U >
constexpr T Num_DigitToChar( U d ) noexcept
{
	return static_cast< T >( d < 10u ? ( '0' + d ) : ( 'A' + ( d - 10u ) ) );
}

///-----------------------------------------------------------------------------
/// @brief Write unsigned @p u in base-NS into @p pOut as exactly @p nDigits chars.
///        Writes **back-to-front** (pOut[nDigits-1] .. pOut[0]).
///        Precondition: nDigits == Num_DigitsNS<I, NS>(u).
///
/// Notes for compile-time friendliness:
///  - The loop runs exactly @p nDigits steps (no while(u)), which removes
///    data-dependent control flow from constexpr paths.
///  - For power-of-two bases, digit step uses shifts & a constexpr mask.
///  - For base-10 and generic base, uses one division per step and computes
///    the remainder as r = u - q*BASE.
///-----------------------------------------------------------------------------
template < typename T = uchar_t, typename I = uint_t, uint8_t NS, typename U >
constexpr T *Num_WriteUnsigned( U u, T *pOut, I nDigits ) noexcept
{
	static_assert( NS >= 2 && NS <= 36, "NS must be in [2..36]" );

	T *q = pOut + nDigits;

	if constexpr ( NS == 16 )
	{
		// Base-16: fixed 4-bit step.
		for ( I i = 0; i < nDigits; ++i )
		{
			const uint_t nib = static_cast< uint_t >( u & U( 0xFu ) );

			*--q = Num_DigitToChar< T, I, U >( nib );
			u >>= 4u;
		}
	}
	else if constexpr ( NS == 10 )
	{
		// Base-10: one division per step; remainder via q*10.
		for ( I i = 0; i < nDigits; ++i )
		{
			const U q10 = u / U( 10u );
			const U r10 = u - q10 * U( 10u );

			*--q = Num_DigitToChar< T, I, U >( static_cast< uint_t >( r10 ) );
			u = q10;
		}
	}
	else if constexpr ( ( NS & ( NS - 1 ) ) == 0 )
	{
		// Any power-of-two base: use shifts & masks.
		constexpr uint_t k = Num_BitWidth_Const( NS );

		constexpr U MASK = ( U( 1 ) << k ) - U( 1 );

		for ( I i = 0; i < nDigits; ++i )
		{
			*--q = Num_DigitToChar< T, I, U >( static_cast< uint_t >( u & MASK ) );
			u >>= k;
		}
	}
	else
	{
		// Generic base: one division per step; remainder via q*BASE.
		constexpr U BASE = U( NS );

		for ( I i = 0; i < nDigits; ++i )
		{
			const U qB = u / BASE;
			const U rB = u - qB * BASE;
			*--q = Num_DigitToChar< T, I, U >( static_cast< uint_t >( rB ) );
			u = qB;
		}
	}

	// Precondition guarantees q == pOut here.
	return pOut;
}

//-----------------------------------------------------------------------------
// (Optional) consteval helper for hard compile-time contexts
//-----------------------------------------------------------------------------
template < uint8_t NS, typename U >
consteval uint_t Num_DigitsNS_Const( U u )
{
	return Num_DigitsNS< uint_t, NS, U >( u );
}

#endif // !defined( _INCLUDE_BALL_TYPES_NUMBER_HPP_ )
