#ifndef _INCLUDE_BALL_TYPES_META_FIBONACCI_HPP_
#	define _INCLUDE_BALL_TYPES_META_FIBONACCI_HPP_

#	pragma once

#	include "fixed.hpp"

// Local shorthand for the 64-bit reference word the derivations below compute in. 
// Defined here (rather than pulled from the arch type layer) so this meta header 
// stays self-contained.
using fib_t = unsigned long long;

///-----------------------------------------------------------------------------
/// @brief The @p nIndex -th Fibonacci number `F(nIndex)`, expanded on demand.
///
/// @details `F(0) = 0`, `F(1) = 1`, `F(n) = F(n-1) + F(n-2)`, folded iteratively 
/// so any index resolves without a fixed lookup table -- "dynamically expandable" 
/// up to the 64-bit ceiling (`F(93)` is the largest that fits; `F(94)` overflows). 
/// The result is a core constant expression, so it can parameterize a template -- 
/// `CFibonacciHash` uses `Fibonacci_NumberConst( 90 )` as its default fold seed.
///
/// @tparam I Fibonacci sequence index type.
///
/// @complexity O(nIndex): one addition per step, resolved at translation time when
/// the index is a constant expression.
///-----------------------------------------------------------------------------
template < typename I >
constexpr fib_t Fibonacci_Number( I nIndex ) noexcept
{
	fib_t nPrevious = 0;
	fib_t nCurrent = 1;

	for ( I i = I( 0 ); i < nIndex; ++i )
	{
		const fib_t nNext = nPrevious + nCurrent;

		nPrevious = nCurrent;
		nCurrent = nNext;
	}

	return nPrevious;
}

///-----------------------------------------------------------------------------
/// @brief Immediate-function form of @ref Fibonacci_Number.
///
/// @details Forces the requested Fibonacci number to be evaluated during
/// translation and rejects run-time arguments.
///
/// @param nIndex Zero-based Fibonacci sequence index.
/// @return `F(nIndex)`.
///
/// @tparam I Fibonacci sequence index type.
///
/// @complexity O(nIndex): one addition per step at translation time.
///-----------------------------------------------------------------------------
template < typename I >
consteval fib_t Fibonacci_NumberConst( I nIndex ) noexcept
{
	return Fibonacci_Number( nIndex );
}

///-----------------------------------------------------------------------------
/// @brief Computes the 64-bit FNV-style fold offset basis for this library.
///
/// @details The FNV specification derives its offset basis as the FNV-0 hash 
/// (seed 0, multiply-then-xor) of Landon Curt Noll's signature string. This uses 
/// the identical construction but over the library's own signature, so the fold 
/// seed is branded to BTL rather than the canonical FNV constant -- a distinct yet 
/// equally well-distributed basis, derived rather than pasted.
///
/// @complexity O(n) in the length of the fixed signature.
///-----------------------------------------------------------------------------
consteval fib_t Fibonacci_FnvOffsetBasis64() noexcept
{
	const char szSignature[] = "BTL";

	constexpr fib_t nPrime = 0x100000001b3ull; // The 64-bit FNV prime.

	fib_t nBasis = 0ull;

	for ( size_t i = 0; szSignature[ i ] != '\0'; ++i )
		nBasis = ( nBasis * nPrime ) ^ static_cast< fib_t >( static_cast< unsigned char >( szSignature[ i ] ) );

	return nBasis;
}

static_assert( Fibonacci_FnvOffsetBasis64(), "the fold offset basis must be nonzero" );

///-----------------------------------------------------------------------------
/// @brief Compile-time constants of the Fibonacci (golden-ratio) hashing scheme, 
/// sized to the width of the working hash word @p I.
///
/// @details Every quantity the hash policy needs -- the multiplier that 
/// approximates `2^N / phi` and the FNV-style fold seed -- is expressed in terms 
/// of the abstract bit width `N = MFixedMetadata< U >::BITS` rather than hard-coded per
/// type.
///
/// The canonical 64-bit multiplier and the compile-time-derived BTL fold basis 
/// are carved to the working width by taking their top @p N bits. The multiplier 
/// is additionally forced odd, so multiplication by it is a bijection modulo 
/// `2^N` and no bucket is unreachable. The same `consteval` path covers every 
/// width from 1 through 64 bits. For the four canonical storage widths this 
/// reproduces the classic constants exactly:
///   -  8 bit -> 0x9F
///   - 16 bit -> 0x9E37
///   - 32 bit -> 0x9E3779B9
///   - 64 bit -> 0x9E3779B97F4A7C15
///
/// @tparam I Unsigned integral hash word type (`uint8_t`, `uint16_t`, 
/// `uint32_t`, `uint64_t`, `size_t`, ... from the BTL fixed-width families).
///
/// @complexity All members are `static constexpr`; every lookup is O(1) and 
/// resolves at translation time.
///-----------------------------------------------------------------------------
template < typename U >
struct MFibonacci
{
	using Fixed_t = MFixedMetadata< U >;

	/// @brief Logical width `N` of the hash word in bits.
	static constexpr bits_t BITS = Fixed_t::BITS;

	static_assert( Fixed_t::IS_UNSIGNED, "MFibonacci requires an unsigned hash word type (signed overflow is undefined)" );

	/// @brief Bit width of the reference golden-ratio / FNV expansions the working
	/// constants are carved from.
	static constexpr bits_t REFERENCE_BITS = static_cast< bits_t >( sizeof( fib_t ) * 8u );

	static_assert( 1 <= BITS && BITS <= REFERENCE_BITS, "MFibonacci supports 1..64-bit hash words" );

	///-----------------------------------------------------------------------------
	/// @brief Computes the odd golden-ratio multiplier `A_N` for the working type
	/// @p U by carving the top `BITS` bits out of the canonical 64-bit reference.
	///
	/// @details `consteval`: a pure function of the width, forced to materialize at
	/// translation time. Forcing the low bit set keeps multiplication a bijection
	/// modulo `2^N`.
	///
	/// @complexity O(1): one shift and one bit-set, all at compile time.
	///-----------------------------------------------------------------------------
	static consteval U ReferenceMultiplier() noexcept
	{
		constexpr fib_t nReferenceMultiplier = 0x9E3779B97F4A7C15ull;

		return static_cast< U >( ( nReferenceMultiplier >> ( REFERENCE_BITS - BITS ) ) | 1ull );
	}

	///-----------------------------------------------------------------------------
	/// @brief Computes the FNV fold seed for the working type @p U by carving the
	/// top `BITS` bits out of the derived 64-bit basis.
	///
	/// @complexity O(1): one shift, all at compile time.
	///-----------------------------------------------------------------------------
	static consteval U ReferenceOffset() noexcept
	{
		return static_cast< U >( Fibonacci_FnvOffsetBasis64() >> ( REFERENCE_BITS - BITS ) );
	}

	/// @brief Odd golden-ratio multiplier `A_N` as the unsigned working word.
	static constexpr U MULTIPLIER = ReferenceMultiplier();

	/// @brief Fold accumulator seed for composite (multi-unit) keys.
	static constexpr U OFFSET_BASIS = ReferenceOffset();

	static_assert( MULTIPLIER & 1, "The Fibonacci multiplier must be odd to stay a bijection modulo 2^N" );
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_FIBONACCI_HPP_ )
